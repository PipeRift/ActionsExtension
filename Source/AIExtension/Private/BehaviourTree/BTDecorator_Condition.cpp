// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "BTDecorator_Condition.h"

UBTDecorator_Condition::UBTDecorator_Condition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    NodeName = "Condition";

    FlowAbortMode = EBTFlowAbortMode::None;
}

bool UBTDecorator_Condition::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    return true;
}

FString UBTDecorator_Condition::GetStaticDescription() const
{
    return Super::GetStaticDescription();
}

#if WITH_EDITOR

FName UBTDecorator_Condition::GetNodeIconName() const
{
    return FName("BTEditor.Graph.BTNode.Decorator.ReachedMoveGoal.Icon");
}

#endif	// WITH_EDITOR