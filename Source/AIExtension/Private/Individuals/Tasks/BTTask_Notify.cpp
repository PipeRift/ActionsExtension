// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "BTTask_Notify.h"

UBTTask_Notify::UBTTask_Notify()
{}

EBTNodeResult::Type UBTTask_Notify::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AActor* Owner = OwnerComp.GetOwner();

	if (!IsValid(Owner))
	{
		return EBTNodeResult::Failed;
	}

	if (FAINotify::TriggerNotify(*Owner, EventName, Parameters))
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

FString UBTTask_Notify::GetStaticDescription() const
{
	return FString::Printf(TEXT("Event: %s"), *EventName.ToString());
}
