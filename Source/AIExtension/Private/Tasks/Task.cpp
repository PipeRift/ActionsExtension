// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "Task.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"
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

void UTask::Initialize(ITaskOwnerInterface* InTaskOwner)
{
    Owner = Cast<UObject>(InTaskOwner);
    State = ETaskState::NOT_RUN;

    if(UTask* TaskOwner = Cast<UTask>(Owner.Get())){
        //If Owner is a Task, registry this as a children
        TaskOwner->AddChildren(this);
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

void UTask::AddChildren(UTask* ChildrenTask)
{
    ChildrenTasks.Add(MakeShareable(ChildrenTask));
}

void UTask::ReceiveActivate_Implementation() {}

void UTask::FinishTask(bool bSuccess, bool bError) {
    if (!IsActivated() || IsPendingKill())
        return;

    if (bError) {
        State = ETaskState::ERROR;
        return;
    }

    State = bSuccess ? ETaskState::SUCCESS : ETaskState::FAILURE;
    
    ReceiveFinished(bSuccess);

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

UTaskComponent* UTask::GetTaskOwnerComponent_Implementation()
{
    ITaskOwnerInterface* OwnerInterface = Cast<ITaskOwnerInterface>(Owner.Get());
    //Owner will always contain this interface.
    checkf(OwnerInterface, TEXT("Owner should always contain a ITaskOwnerInterface"));

    return OwnerInterface->GetTaskOwnerComponent();
}
