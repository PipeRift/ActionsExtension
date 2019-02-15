// Copyright 2015-2019 Piperift. All Rights Reserved.

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
	UObject const * const Outer = GetOuter();
	if (IsPendingKill() || !IsValid(Outer) || !Outer->Implements<UActionOwnerInterface>() ||
		State != EActionState::PREPARING)
	{
		UE_LOG(ActionLog, Warning, TEXT("Action '%s' is already running or pending destruction."), *GetName());
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
	return ChildrenActions.AddUnique(NewChildren) != INDEX_NONE;
}

const bool UAction::RemoveChildren(UAction* Children)
{
	return ChildrenActions.Remove(Children) > 0;
}

bool UAction::EventCanActivate_Implementation() {
	return true;
}

void UAction::Finish(bool bSuccess) {
	if (!IsRunning() || IsPendingKill())
		return;

	State = bSuccess ? EActionState::SUCCESS : EActionState::FAILURE;
	OnFinish(State);
	GetParentInterface()->RemoveChildren(this);
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
	for (auto* Children : ChildrenActions)
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

UAction* UAction::Create(const TScriptInterface<IActionOwnerInterface>& Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate /*= false*/)
{
	if (!Owner.GetObject())
		return nullptr;

	if (!Type.Get() || Type == UAction::StaticClass())
		return nullptr;

	if (!bAutoActivate)
	{
		return NewObject<UAction>(Owner.GetObject(), Type);
	}
	else
	{
		UAction* Action = NewObject<UAction>(Owner.GetObject(), Type);
		Action->Activate();
		return Action;
	}
}

UAction* UAction::Create(const TScriptInterface<IActionOwnerInterface>& Owner, UAction* Template, bool bAutoActivate /*= false*/)
{
	if (!Owner.GetObject() || !Template)
		return nullptr;

	UClass* const Type = Template->GetClass();
	check(Type);

	if (Type == UAction::StaticClass())
		return nullptr;

	if (!bAutoActivate)
	{
		return NewObject<UAction>(Owner.GetObject(), Type, NAME_None, RF_NoFlags, Template);
	}
	else
	{
		UAction* Action = NewObject<UAction>(Owner.GetObject(), Type, NAME_None, RF_NoFlags, Template);
		Action->Activate();
		return Action;
	}
}
