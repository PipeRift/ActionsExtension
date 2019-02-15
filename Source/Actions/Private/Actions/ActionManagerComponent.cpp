// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "ActionManagerComponent.h"
#include "Action.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger_Actions.h"
#endif // WITH_GAMEPLAY_DEBUGGER


UActionManagerComponent::UActionManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UActionManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	ChildrenActions.Empty();
}

void UActionManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CancelAll();
}

void UActionManagerComponent::CancelAll()
{
	for (auto* Children : ChildrenActions)
	{
		if (Children)
			Children->Cancel();
	}
}

void UActionManagerComponent::CancelByPredicate(TFunctionRef<bool(const UAction*)> Predicate)
{
	for (auto* Children : ChildrenActions)
	{
		if (Children && Predicate(Children)) {
			//Cancel task
			Children->Cancel();
		}
	}
}

const bool UActionManagerComponent::AddChildren(UAction* NewChildren)
{
	return ChildrenActions.AddUnique(NewChildren) != INDEX_NONE;
}

const bool UActionManagerComponent::RemoveChildren(UAction* Children)
{
	return ChildrenActions.Remove(Children) > 0;
}

UActionManagerComponent* UActionManagerComponent::GetActionOwnerComponent() const
{
	return const_cast<UActionManagerComponent*>(this);
}

#if WITH_GAMEPLAY_DEBUGGER
void UActionManagerComponent::DescribeSelfToGameplayDebugger(const FName& BaseName, FGameplayDebugger_Actions& Debugger) const
{
	const AActor* Owner = GetOwner();
	static const FString StateColorText = TEXT("{green}");

	Debugger.AddTextLine(FString::Printf(TEXT("%s%s: %s"), *StateColorText, *BaseName.ToString(), *Owner->GetName()));

	for (const auto* Action : ChildrenActions)
	{
		DescribeActionToGameplayDebugger(Action, Debugger, 1);
	}

	Debugger.AddTextLine(TEXT(""));
}

void UActionManagerComponent::DescribeActionToGameplayDebugger(const UAction* Action, FGameplayDebugger_Actions& Debugger, int8 Indent) const
{
	if (Action)
	{
		FString ColorText = TEXT("");
		switch (Action->GetState())
		{
		case EActionState::RUNNING:
			ColorText = TEXT("{cyan}");
			break;
		case EActionState::SUCCESS:
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

		const FString CanceledSuffix = Action->GetState() == EActionState::CANCELED ? TEXT("CANCELLED") : TEXT("");

		if (Action->IsRunning())
		{
			Debugger.AddTextLine(FString::Printf(TEXT("%s%s>%s %s"), *IndentString, *ColorText, *Action->GetName(), *CanceledSuffix));

			for (const auto* ChildAction : Action->ChildrenActions)
			{
				DescribeActionToGameplayDebugger(ChildAction, Debugger, Indent + 1);
			}
		}
	}
}

#endif // WITH_GAMEPLAY_DEBUGGER

