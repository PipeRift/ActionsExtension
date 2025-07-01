// Copyright 2015-2023 Piperift. All Rights Reserved.

#include "ActionsSubsystem.h"

#include "Action.h"

#include <GameFramework/WorldSettings.h>


#if WITH_GAMEPLAY_DEBUGGER
#	include "GameplayDebugger_Actions.h"
#endif	  // WITH_GAMEPLAY_DEBUGGER


void FActionsTickGroup::Tick(float DeltaTime)
{
	if (Actions.Num() <= 0)
	{
		return;
	}

	if (TickRate > KINDA_SMALL_NUMBER)
	{
		TickTimeElapsed += DeltaTime;
		if (TickTimeElapsed < TickRate)
		{
			return;
		}

		// Delayed Tick
		DelayedTick(TickTimeElapsed);

		TickTimeElapsed = 0.f;
	}
	else
	{
		// Normal Tick
		DelayedTick(DeltaTime);
	}
}

void FActionsTickGroup::DelayedTick(float DeltaTime)
{
	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		auto* const Action = Actions[i];
		if (!Action)
		{
			Actions.RemoveAtSwap(i, 1, EAllowShrinking::No);
			--i;
		}
		else if (Action->CanTick())
		{
			Action->DoTick(DeltaTime);
		}
	}
}

void FActionOwner::CancelAll(bool bShouldShrink)
{
	for (auto& Action : Actions)
	{
		if (Action)
		{
			Action->Cancel();
		}
	}

	if (bShouldShrink)
	{
		Actions.Empty();
	}
	else
	{
		Actions.Reset();
	}
}

void FActionOwner::CancelByPredicate(const TFunctionRef<bool(const UAction*)>& Predicate, bool bShouldShrink)
{
	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		auto* Action = Actions[i].Get();
		if (Action && Predicate(Action))
		{
			// Cancel action
			Action->Cancel();

			// Remove action
			Actions.RemoveAtSwap(i, 1, EAllowShrinking::No);
			--i;
		}
	}

	if (bShouldShrink)
	{
		Actions.Shrink();
	}
}


void UActionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UActionsSubsystem::Deinitialize()
{
	CancelAll();
	Super::Deinitialize();
}

void UActionsSubsystem::Tick(float DeltaTime)
{
	// Cancel destroyed object actions or of which the outer is invalid
	for (auto RootIt = ActionOwners.CreateIterator(); RootIt; ++RootIt)
	{
		if (!RootIt->Owner.IsValid())
		{
			RootIt->CancelAll(false);
			RootIt.RemoveCurrent();
		}
		else
		{
			// Remove garbage collected actions
			RootIt->Actions.RemoveAllSwap(
				[](const UAction* Action) {
					return !IsValid(Action);
				},
				EAllowShrinking::No);

			if (RootIt->Actions.Num() <= 0)
			{
				RootIt.RemoveCurrent();
			}
		}
	}

	// Tick all tick groups
	const float TimeDilation = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
	for (int32 i = 0; i < TickGroups.Num(); ++i)
	{
		auto& TickGroup = TickGroups[i];

		TickGroup.Tick(DeltaTime * TimeDilation);

		if (TickGroup.Actions.Num() <= 0)
		{
			// Tick group is empty so we remove and ignore it
			TickGroups.RemoveAtSwap(i, 1, EAllowShrinking::No);
			--i;
		}
	}
	TickGroups.Shrink();
}

void UActionsSubsystem::CancelAll()
{
	for (auto& RootAction : ActionOwners)
	{
		RootAction.CancelAll(false);
	}
	ActionOwners.Reset();
}

void UActionsSubsystem::CancelAllByOwner(UObject* Object)
{
	const FSetElementId OwnerId = ActionOwners.FindId(Object);
	if (OwnerId.IsValidId())
	{
		ActionOwners[OwnerId].CancelAll(false);
		ActionOwners.Remove(OwnerId);
	}
}

void UActionsSubsystem::CancelByPredicate(TFunctionRef<bool(const UAction*)> Predicate)
{
	for (auto& RootAction : ActionOwners)
	{
		RootAction.CancelByPredicate(Predicate);
	}
}

void UActionsSubsystem::CancelByOwnerPredicate(UObject* Object, TFunctionRef<bool(const UAction*)> Predicate)
{
	if (FActionOwner* const Owner = ActionOwners.Find(Object))
	{
		Owner->CancelByPredicate(Predicate);
	}
}

void UActionsSubsystem::AddRootAction(UAction* Child)
{
	check(Child);

	// Registry for GC Canceling
	UObject* Owner = Child->GetOuter();

	FSetElementId OwnerId = ActionOwners.FindId(Owner);
	if (!OwnerId.IsValidId())
	{
		OwnerId = ActionOwners.Add({Owner});
	}
	ActionOwners[OwnerId].Actions.Add(Child);
}

void UActionsSubsystem::AddActionToTickGroup(UAction* Child)
{
	const float TickRate = Child->GetTickRate();

	// Registry for tick groups
	FActionsTickGroup* Group = TickGroups.FindByKey(TickRate);
	if (!Group)
	{
		TickGroups.Add({TickRate});
		Group = &TickGroups.Last();
	}

	Group->Actions.Add(Child);
}

void UActionsSubsystem::RemoveActionFromTickGroup(UAction* Child)
{
	const float TickRate = Child->GetTickRate();
	int32 Index = TickGroups.Find(TickRate);
	if (Index != INDEX_NONE)
	{
		auto& Group = TickGroups[Index];
		if (Group.Actions.Num() > 1)
		{
			Group.Actions.RemoveSwap(Child, EAllowShrinking::No);
		}
		else
		{
			TickGroups.RemoveAtSwap(Index);
		}
	}
}

#if WITH_GAMEPLAY_DEBUGGER
void UActionsSubsystem::DescribeOwnerToGameplayDebugger(
	UObject* Owner, const FName& BaseName, FGameplayDebugger_Actions& Debugger) const
{
	static const FString StateColorText = TEXT("{green}");

	Debugger.AddTextLine(
		FString::Printf(TEXT("%s%s: %s"), *StateColorText, *BaseName.ToString(), *Owner->GetName()));

	if (const FActionOwner* const RootAction = ActionOwners.Find(Owner))
	{
		for (const UAction* Action : RootAction->Actions)
		{
			if (IsValid(Action))
			{
				Action->DescribeSelfToGameplayDebugger(Debugger, 1);
			}
		}
	}

	Debugger.AddTextLine(TEXT(""));
}
#endif	  // WITH_GAMEPLAY_DEBUGGER
