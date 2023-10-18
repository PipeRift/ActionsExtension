// Copyright 2015-2023 Piperift. All Rights Reserved.

#include "Action.h"

#include "TimerManager.h"

#include <Components/ActorComponent.h>
#include <Engine/EngineTypes.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>


#if WITH_GAMEPLAY_DEBUGGER
#	include "GameplayDebugger_Actions.h"
#endif	  // WITH_GAMEPLAY_DEBUGGER

DEFINE_LOG_CATEGORY(ActionLog);


UAction* CreateAction(UObject* Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate /*= false*/)
{
	if (!IsValid(Owner) || !Type.Get() || Type == UAction::StaticClass())
	{
		return nullptr;
	}

	UAction* Action = NewObject<UAction>(Owner, Type);
	if (bAutoActivate)
	{
		Action->Activate();
	}
	return Action;
}

UAction* CreateAction(UObject* Owner, UAction* Template, bool bAutoActivate /*= false*/)
{
	if (!IsValid(Owner) || !Template)
	{
		return nullptr;
	}

	UClass* const Type = Template->GetClass();
	check(Type);

	if (Type == UAction::StaticClass())
	{
		return nullptr;
	}

	UAction* Action = NewObject<UAction>(Owner, Type, NAME_None, RF_NoFlags, Template);
	if (bAutoActivate)
	{
		Action->Activate();
	}
	return Action;
}


void UAction::Activate()
{
	UActionsSubsystem* Subsystem = GetSubsystem();

	if (!IsValid(this) || !IsValid(GetOuter()) || State != EActionState::Preparing)
	{
		UE_LOG(
			ActionLog, Warning, TEXT("Action '%s' is already running or pending destruction."), *GetName());
		Destroy();
		return;
	}

	if (!CanActivate())
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

	if (bWantsToTick)
	{
		Subsystem->AddActionToTickGroup(this);
	}

	State = EActionState::Running;
	OnActivation();
}

void UAction::Cancel()
{
	if (!IsValid(this))
		return;

	if (!IsRunning())
	{
		Destroy();
		return;
	}

	OnFinish(State = EActionState::Cancelled);
	Destroy();
}

void UAction::OnFinish(const EActionState Reason)
{
	// Stop any timers or latent actions for the action
	if (UWorld* World = GetWorld())
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

void UAction::Finish(bool bSuccess)
{
	if (!IsRunning() || !IsValid(this))
		return;

	State = bSuccess ? EActionState::Success : EActionState::Failure;
	OnFinish(State);

	// Remove from parent action
	if (auto* ParentAction = GetParentAction())
	{
		ParentAction->RemoveChildren(this);
	}

	Destroy();
}

void UAction::Destroy()
{
	if (!IsValid(this))
		return;

	// Cancel and destroy all children tasks
	for (auto* Children : ChildrenActions)
	{
		if (Children)
		{
			Children->Cancel();
		}
	}
	ChildrenActions.Reset();

	MarkAsGarbage();
}

void UAction::AddChildren(UAction* Child)
{
	ChildrenActions.Add(Child);
}

void UAction::RemoveChildren(UAction* Child)
{
	ChildrenActions.RemoveSwap(Child, false);
}

bool UAction::ReceiveCanActivate_Implementation()
{
	return true;
}

bool UAction::GetWantsToTick() const
{
	return bWantsToTick;
}

bool UAction::IsRunning() const
{
	return State == EActionState::Running;
}

bool UAction::Succeeded() const
{
	return State == EActionState::Success;
}

bool UAction::Failed() const
{
	return State == EActionState::Failure;
}

EActionState UAction::GetState() const
{
	return State;
}

UObject* const UAction::GetParent() const
{
	return GetOuter();
}

UAction* UAction::GetParentAction() const
{
	return Cast<UAction>(GetOuter());
}

float UAction::GetTickRate() const
{
	// Reduce TickRate Precision to 0.1ms
	return FMath::FloorToFloat(TickRate * 10000.f) * 0.0001f;
}

UObject* UAction::GetOwner() const
{
	return Owner.Get();
}

AActor* UAction::GetOwnerActor() const
{
	// With this function we can predict the owner is more likely to be an actor so we check it first
	if (AActor* Actor = Cast<AActor>(Owner))
	{
		return Actor;
	}
	else if (auto* Component = Cast<UActorComponent>(Owner))
	{
		return Component->GetOwner();
	}
	return nullptr;
}

UActorComponent* UAction::GetOwnerComponent() const
{
	return Cast<UActorComponent>(Owner);
}

UWorld* UAction::GetWorld() const
{
	// If we are a CDO, we must return nullptr to fool UObject::ImplementsGetWorld
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	if (const UObject* InOwner = GetOwner())
	{
		InOwner->GetWorld();
	}
	return nullptr;
}

#if WITH_GAMEPLAY_DEBUGGER
void UAction::DescribeSelfToGameplayDebugger(FGameplayDebugger_Actions& Debugger, int8 Indent) const
{
	FString ColorText = TEXT("");
	switch (State)
	{
		case EActionState::Running:
			ColorText = TEXT("{cyan}");
			break;
		case EActionState::Success:
			ColorText = TEXT("{green}");
			break;
		default:
			ColorText = TEXT("{red}");
	}

	FString IndentString = "";
	for (int32 I = 0; I < Indent; ++I)
	{
		IndentString += "  ";
	}

	const FString CanceledSuffix = (State == EActionState::Cancelled) ? TEXT("CANCELLED") : FString{};

	if (IsRunning())
	{
		Debugger.AddTextLine(
			FString::Printf(TEXT("%s%s>%s %s"), *IndentString, *ColorText, *GetName(), *CanceledSuffix));

		for (const auto* ChildAction : ChildrenActions)
		{
			if (ChildAction)
			{
				ChildAction->DescribeSelfToGameplayDebugger(Debugger, Indent + 1);
			}
			else
			{
				ensureMsgf(false, TEXT("Invalid child on action %s while debugging"), *GetName());
			}
		}
	}
}
#endif	  // WITH_GAMEPLAY_DEBUGGER

void UAction::SetWantsToTick(bool bValue)
{
	if (bValue != bWantsToTick)
	{
		bWantsToTick = bValue;
		UActionsSubsystem* Subsystem = GetSubsystem();
		if (bValue)
		{
			Subsystem->AddActionToTickGroup(this);
		}
		else
		{
			Subsystem->RemoveActionFromTickGroup(this);
		}
	}
}

void UAction::PostInitProperties()
{
	Super::PostInitProperties();

	UObject* Outer = GetOuter();
	if (UAction* Parent = Cast<UAction>(Outer))
	{
		Owner = Parent->GetOwner();
	}
	else
	{
		Owner = Outer;
	}
}

UActionsSubsystem* UAction::GetSubsystem() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return UGameInstance::GetSubsystem<UActionsSubsystem>(GI);
}
