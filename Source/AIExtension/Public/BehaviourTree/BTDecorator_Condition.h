// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BehaviorTree/BTDecorator.h"

#include "Conditions/Condition.h"

#include "BTDecorator_Condition.generated.h"

/**
* Condition decorator node.
*/
UCLASS()
class AIEXTENSION_API UBTDecorator_Condition : public UBTDecorator
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(EditAnywhere, Instanced, Category = Node)
    UCondition* Condition;
	
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
    virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
};
