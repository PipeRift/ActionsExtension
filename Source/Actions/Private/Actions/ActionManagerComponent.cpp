// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "ActionManagerComponent.h"
#include "Action.h"


UActionManagerComponent::UActionManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UActionManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	ChildrenTasks.Empty();
}

void UActionManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CancelAll();
}

void UActionManagerComponent::CancelAll()
{
	for (auto* Children : ChildrenTasks)
	{
		if (Children)
			Children->Cancel();
	}
}

void UActionManagerComponent::CancelByPredicate(TFunctionRef<bool(const UAction*)> Predicate)
{
	for (auto* Children : ChildrenTasks)
	{
		if (Children && Predicate(Children)) {
			//Cancel task
			Children->Cancel();
		}
	}
}

const bool UActionManagerComponent::AddChildren(UAction* NewChildren)
{
	return ChildrenTasks.AddUnique(NewChildren) != INDEX_NONE;
}

const bool UActionManagerComponent::RemoveChildren(UAction* Children)
{
	return ChildrenTasks.Remove(Children) > 0;
}

UActionManagerComponent* UActionManagerComponent::GetActionOwnerComponent() const
{
	return const_cast<UActionManagerComponent*>(this);
}

