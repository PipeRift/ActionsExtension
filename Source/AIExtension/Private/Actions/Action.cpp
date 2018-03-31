// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "Action.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "ActionManagerComponent.h"
#include "GameplayTaskOwnerInterface.h"

DEFINE_LOG_CATEGORY(TaskLog);

UAction::UAction(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    TickTimeElapsed = 0;

    State = EActionState::NOT_RUN;

    // SETUP PROPERTIES
    bWantsToTick = false;
    TickRate = 0.15f;
}

void UAction::Activate()
{
    IActionOwnerInterface* const Parent = GetParentInterface();
    if (!Parent) {
        UE_LOG(TaskLog, Error, TEXT("Task's Outer must have a TaskOwnerInterface! Detroying for safety."));
        Destroy();
        return;
    }

    if (!IsValid() || IsRunning())
        return;

    //Registry this children task in the owner
    const bool bSuccess = Parent->AddChildren(this);
    if (!bSuccess) {
        GetActionOwnerComponent()->AddChildren(this);
    }

    State = EActionState::RUNNING;
    
    OnActivation();
}

const bool UAction::AddChildren(UAction* NewChildren)
{
    return ChildrenTasks.AddUnique(NewChildren) != INDEX_NONE;
}

const bool UAction::RemoveChildren(UAction* Children)
{
    return ChildrenTasks.Remove(Children) > 0;
}

void UAction::ReceiveActivate_Implementation() {
    //Finish by default
    Finish(true);
}


void UAction::Finish(bool bSuccess) {
    if (!IsRunning() || IsPendingKill())
        return;

    State = bSuccess ? EActionState::SUCCESS : EActionState::FAILURE;
    OnFinish(State);
    Destroy();
}

void UAction::Abort() {
    if (!IsRunning() || IsPendingKill())
        return;

    OnFinish(State = EActionState::ABORTED);

    Destroy();
}


void UAction::Cancel()
{
    if (IsPendingKill())
        return;

    if(!IsRunning())
    {
        Destroy();
        return;
    }

    OnFinish(State = EActionState::CANCELED);
    
    Destroy();
}

void UAction::Destroy()
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

void UAction::Tick(float DeltaTime)
{
    if (TickRate > 0) {
        TickTimeElapsed += DeltaTime;
        if (TickTimeElapsed < TickRate)
            return;

        // Delayed Tick
        TaskTick(TickTimeElapsed);
        ReceiveTick(TickTimeElapsed);

        TickTimeElapsed = 0;
    } else {
        //Normal Tick
        TaskTick(DeltaTime);
        ReceiveTick(DeltaTime);
    }
}

void UAction::OnFinish(const EActionState Reason)
{
    OnTaskFinished.Broadcast(Reason);

    const UObject* const Parent = GetParent();

    // If we're in the process of being garbage collected it is unsafe to call out to blueprints
    if (Parent && !Parent->HasAnyFlags(RF_BeginDestroyed) && !Parent->IsUnreachable())
    {
        ReceiveFinished(Reason);
    }
}

AActor* UAction::GetActionOwnerActor()
{
    return GetActionOwnerComponent()->GetOwner();
}

UActionManagerComponent* UAction::GetActionOwnerComponent()
{
    return GetParentInterface()->GetActionOwnerComponent();
}
