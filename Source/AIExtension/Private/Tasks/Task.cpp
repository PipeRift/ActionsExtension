// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "Task.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "TaskManagerComponent.h"
#include "GameplayTaskOwnerInterface.h"


UTask::UTask(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    TickTimeElapsed = 0;

    State = ETaskState::NOT_RUN;

    // SETUP PROPERTIES
    bWantsToTick = false;
    TickRate = 0.15f;
}

void UTask::Initialize(ITaskOwnerInterface* InTaskParent)
{
    Parent = InTaskParent;
    State = ETaskState::NOT_RUN;

    //Registry this children task in the owner
    bool bRegistryResult = Parent->AddChildren(this);
    if (!bRegistryResult) {
        GetTaskOwnerComponent()->AddChildren(this);
    }
}

void UTask::Activate()
{
    if (!IsInitialized() || IsActivated() || IsPendingKill())
        return;

    State = ETaskState::RUNNING;
    
    OnActivation();
    //Freezes the game
    ReceiveActivate();
}

const bool UTask::AddChildren(UTask* NewChildren)
{
    ChildrenTasks.Add(MakeShareable(NewChildren));
    return true;
}

const bool UTask::RemoveChildren(UTask* Children)
{
    ChildrenTasks.Remove(MakeShareable(Children));
    return true;
}

void UTask::ReceiveActivate_Implementation() {}

void UTask::Finish(bool bSuccess, bool bError) {
    if (!IsActivated() || IsPendingKill())
        return;

    if (bError) {
        State = ETaskState::ERROR;
        Destroy();
        return;
    }

    State = bSuccess ? ETaskState::SUCCESS : ETaskState::FAILURE;
    
    ReceiveFinished(bSuccess);

    Destroy();
}

void UTask::Cancel()
{
    if (!IsActivated())
        return;

    State = ETaskState::CANCELED;

    ReceiveCanceled();

    Destroy();
}

void UTask::Destroy()
{
    //Cancel and destroy all children tasks
    for (auto& Children : ChildrenTasks)
    {
        if (Children.IsValid()) {
            Children->Cancel();
        }
    }
    ChildrenTasks.Empty();

    //RemoveFromRoot();
    //Mark for destruction
    MarkPendingKill();
}

void UTask::Tick(float DeltaTime)
{
    if (TickRate > 0) {
        TickTimeElapsed += DeltaTime;
        if (TickTimeElapsed < TickRate)
            return;

        // Limited Tick
        TaskTick(TickTimeElapsed);
        ReceiveTick(TickTimeElapsed);

        TickTimeElapsed = 0;
    } else {
        //Normal Tick
        TaskTick(DeltaTime);
        ReceiveTick(DeltaTime);
    }
}

UTaskManagerComponent* UTask::GetTaskOwnerComponent_Implementation()
{
    //Owner will always contain this interface.
    checkf(Parent, TEXT("Owner should always contain a ITaskOwnerInterface"));

    return Parent->GetTaskOwnerComponent();
}

UTaskManagerComponent* UTask::ExposedGetTaskOwnerComponent()
{
    return GetTaskOwnerComponent();
}

AActor* UTask::GetTaskOwnerActor()
{
    return GetTaskOwnerComponent()->GetOwner();
}
