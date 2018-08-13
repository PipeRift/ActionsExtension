// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "Action.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "ActionManagerComponent.h"
#include "GameplayTaskOwnerInterface.h"

DEFINE_LOG_CATEGORY(ActionLog);

UAction::UAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TickTimeElapsed = 0;

	State = EActionState::PREPARING;

	// Default Properties
	bWantsToTick = false;
	TickRate = 0.15f;
}

void UAction::Activate()
{
	if (!IsValid() || State != EActionState::PREPARING)
	{
		UE_LOG(ActionLog, Warning, TEXT("Action is already running or pending destruction."));
		return;
	}

	IActionOwnerInterface* const Parent = GetParentInterface();
	if (!Parent) {
		UE_LOG(ActionLog, Error, TEXT("Action must have a valid parent (Action or ActionManager). Detroying for safety."));
		Destroy();
		return;
	}

	if(!CanActivate())
	{
		UE_LOG(ActionLog, Error, TEXT("Could not activate. CanActivate() Failed."));
		Destroy();
		return;
	}

	//Registry this children task in the owner
	const bool bSuccess = Parent->AddChildren(this);
	if (!bSuccess)
	{
		UActionManagerComponent* Manager = GetActionOwnerComponent();
		if (!Manager)
		{
			Destroy();
			return;
		}

		Manager->AddChildren(this);
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

bool UAction::EventCanActivate_Implementation() {
	return true;
}

void UAction::Finish(bool bSuccess) {
	if (!IsRunning() || IsPendingKill())
		return;

	State = bSuccess ? EActionState::SUCCESS : EActionState::FAILURE;
	OnFinish(State);
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
		ActionTick(TickTimeElapsed);
		ReceiveTick(TickTimeElapsed);

		TickTimeElapsed = 0;
	} else {
		//Normal Tick
		ActionTick(DeltaTime);
		ReceiveTick(DeltaTime);
	}
}

void UAction::OnFinish(const EActionState Reason)
{
	OnFinishedDelegate.Broadcast(Reason);

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

UActionManagerComponent* UAction::GetActionOwnerComponent() const
{
	return GetParentInterface()->GetActionOwnerComponent();
}
