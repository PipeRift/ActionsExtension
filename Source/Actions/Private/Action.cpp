// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Action.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "GameplayTaskOwnerInterface.h"

DEFINE_LOG_CATEGORY(ActionLog);

UAction::UAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	State = EActionState::PREPARING;

	// Default Properties
	bWantsToTick = false;
	TickRate = 0.15f;
}

void UAction::Activate()
{
	UActionsSubsystem* Subsystem = GetSubsystem();

	if (IsPendingKill() || !IsValid(GetOuter()) || State != EActionState::PREPARING)
	{
		UE_LOG(ActionLog, Warning, TEXT("Action '%s' is already running or pending destruction."), *GetName());
		Destroy();
		return;
	}

	if(!CanActivate())
	{
		UE_LOG(ActionLog, Log, TEXT("Could not activate. CanActivate() Failed."));
		Destroy();
		return;
	}

	// Add this action to its parent
	if (auto* ParentAction = GetParentAction())
	{
		ParentAction->AddChildren(this);
	}
	else
	{
		Subsystem->AddRootAction(this);
	}

	Subsystem->AddActionToTickGroup(this);

	State = EActionState::RUNNING;
	OnActivation();
}

bool UAction::EventCanActivate_Implementation() {
	return true;
}

void UAction::Finish(bool bSuccess) {
	if (!IsRunning() || IsPendingKill())
		return;

	State = bSuccess ? EActionState::SUCCESS : EActionState::FAILURE;
	OnFinish(State);

	//Remove from parent action
	if (auto* ParentAction = GetParentAction())
	{
		ParentAction->RemoveChildren(this);
	}

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

	//Cancel and destroy all children tasks
	for (auto* Children : ChildrenActions)
	{
		if (Children) {
			Children->Cancel();
		}
	}
	ChildrenActions.Reset();

	//Mark for destruction
	MarkPendingKill();
}

void UAction::AddChildren(UAction* Child)
{
	ChildrenActions.Add(Child);
}

void UAction::RemoveChildren(UAction* Child)
{
	ChildrenActions.Remove(Child);
}

void UAction::OnFinish(const EActionState Reason)
{
	// Stop any timers or latent actions for the action
	if (UWorld * World = GetWorld())
	{
		World->GetLatentActionManager().RemoveActionsForObject(this);
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	OnFinishedDelegate.Broadcast(Reason);

	const UObject* Parent = GetParent();
	// If we're in the process of being garbage collected it is unsafe to call out to blueprints
	if (Parent && !Parent->HasAnyFlags(RF_BeginDestroyed) && !Parent->IsUnreachable())
	{
		ReceiveFinished(Reason);
	}
}

UObject* UAction::GetOwner() const
{
	UObject* Owner = nullptr;
	const UAction* Current = this;

	// #TODO: Ensure this works
	while (Current)
	{
		Owner = Current->GetParent();
		Current = Current->GetParentAction();
	}
	return Owner;
}

AActor* UAction::GetOwnerActor() const
{
	UObject* const Owner = GetOwner();
	if (auto * Component = Cast<UActorComponent>(Owner))
	{
		return Component->GetOwner();
	}
	return Cast<AActor>(Owner);
}

UAction* UAction::Create(UObject* Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate /*= false*/)
{
	if (!IsValid(Owner) || !Type.Get() || Type == UAction::StaticClass())
		return nullptr;

	if (!bAutoActivate)
	{
		return NewObject<UAction>(Owner, Type);
	}
	else
	{
		UAction* Action = NewObject<UAction>(Owner, Type);
		Action->Activate();
		return Action;
	}
}

UAction* UAction::Create(UObject* Owner, UAction* Template, bool bAutoActivate /*= false*/)
{
	if (!IsValid(Owner) || !Template)
		return nullptr;

	UClass* const Type = Template->GetClass();
	check(Type);

	if (Type == UAction::StaticClass())
		return nullptr;

	if (!bAutoActivate)
	{
		return NewObject<UAction>(Owner, Type, NAME_None, RF_NoFlags, Template);
	}
	else
	{
		UAction* Action = NewObject<UAction>(Owner, Type, NAME_None, RF_NoFlags, Template);
		Action->Activate();
		return Action;
	}
}
