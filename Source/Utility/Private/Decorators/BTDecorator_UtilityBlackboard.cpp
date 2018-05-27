// Copyright 2015 Cameron Angus. All Rights Reserved.

#include "Decorators/BTDecorator_UtilityBlackboard.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"


UBTDecorator_UtilityBlackboard::UBTDecorator_UtilityBlackboard(const FObjectInitializer& ObjectInitializer) :
Super(ObjectInitializer)
{
    NodeName = "Blackboard Utility";

    // accept only float and integer keys
    Value.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_UtilityBlackboard, Value));
    Value.AddIntFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_UtilityBlackboard, Value));
}

void UBTDecorator_UtilityBlackboard::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    UBlackboardData* BBAsset = GetBlackboardAsset();
    if (BBAsset)
    {
        Value.ResolveSelectedKey(*BBAsset);
    }
    else
    {
        UE_LOG(LogBehaviorTree, Warning, TEXT("Can't initialize %s due to missing blackboard data."), *GetName());
        Value.InvalidateResolvedKey();
    }
}

float UBTDecorator_UtilityBlackboard::CalculateUtilityValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
    float Utility = 0.0f;

    if (Value.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
    {
        Utility = MyBlackboard->GetValue< UBlackboardKeyType_Float >(Value.GetSelectedKeyID());
    }
    else if (Value.SelectedKeyType == UBlackboardKeyType_Int::StaticClass())
    {
        Utility = (float)MyBlackboard->GetValue< UBlackboardKeyType_Int >(Value.GetSelectedKeyID());
    }

    return FMath::Max(Utility, 0.0f);
}

FString UBTDecorator_UtilityBlackboard::GetStaticDescription() const
{
    return FString::Printf(TEXT("Value: %s"), *GetSelectedBlackboardKey().ToString());
}

void UBTDecorator_UtilityBlackboard::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
    Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

    const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    FString DescKeyValue;

    if (BlackboardComp)
    {
        DescKeyValue = BlackboardComp->DescribeKeyValue(Value.GetSelectedKeyID(), EBlackboardDescription::OnlyValue);
    }

    Values.Add(FString::Printf(TEXT("utility: %s"), *DescKeyValue));
}


