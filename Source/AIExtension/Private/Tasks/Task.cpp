// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "Task.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "TaskManagerComponent.h"
#include "GameplayTaskOwnerInterface.h"

DEFINE_LOG_CATEGORY(TaskLog);

UTask::UTask(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    TickTimeElapsed = 0;

    State = ETaskState::NOT_RUN;

    // SETUP PROPERTIES
    bWantsToTick = false;
    TickRate = 0.15f;
}

void UTask::Activate()
{
    ITaskOwnerInterface* const Parent = GetParentInterface();
    if (!Parent) {
        UE_LOG(TaskLog, Error, TEXT("Task's Outer must have a TaskOwnerInterface! Detroying for safety."));
        Destroy();
        return;
    }

    //Registry this children task in the owner
    const bool bSuccess = Parent->AddChildren(this);
    if (!bSuccess) {
        GetTaskOwnerComponent()->AddChildren(this);
    }

    if (!IsValid() || IsActivated())
        return;

    State = ETaskState::RUNNING;
    
    OnActivation();
    //Freezes the game
    ReceiveActivate();
}

const bool UTask::AddChildren(UTask* NewChildren)
{
    return ChildrenTasks.AddUnique(NewChildren) != INDEX_NONE;
}

const bool UTask::RemoveChildren(UTask* Children)
{
    return ChildrenTasks.Remove(Children) > 0;
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
    const UObject* const Parent = GetParent();

    if (IsActivated()){
        State = ETaskState::CANCELED;

        // If we're in the process of being garbage collected it is unsafe to call out to blueprints
        if (Parent && !Parent->HasAnyFlags(RF_BeginDestroyed) && !Parent->IsUnreachable())
        {
            ReceiveCanceled();
        }
    }

    Destroy();

    if(Parent && !Parent->IsPendingKill()) {
        GetParentInterface()->RemoveChildren(this);
    }
}

void UTask::Destroy()
{
    if (IsPendingKill())
        return;

    //Mark for destruction
    MarkPendingKill();

    //Cancel and destroy all children tasks
    for (auto* Children : ChildrenTasks)
    {
        if (Children) {
            Children->Cancel();
        }
    }
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

UTaskManagerComponent* UTask::GetTaskOwnerComponent()
{
    //Owner will always contain this interface.
    checkf(IsValid(), TEXT("Owner should always have a ITaskOwnerInterface"));

    return GetParentInterface()->GetTaskOwnerComponent();
}

UTaskManagerComponent* UTask::ExposedGetTaskOwnerComponent()
{
    return GetTaskOwnerComponent();
}

AActor* UTask::GetTaskOwnerActor()
{
    return GetTaskOwnerComponent()->GetOwner();
}
