// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "AIGeneric.h"
#include "BTD_CompareState.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECompareStateMode : uint8
{
    Equals         UMETA(DisplayName = "Equals"),
    GreaterThan    UMETA(DisplayName = "Greater Than"),
    LessThan       UMETA(DisplayName = "Less Than"),
    GreaterOrEqual UMETA(DisplayName = "Greater Than or Equal to"),
    LessOrEqual    UMETA(DisplayName = "Less Than or Equal to"),
    NotEqual       UMETA(DisplayName = "Not Equal")
};

UCLASS()
class AIEXTENSION_API UBTD_CompareState : public UBTDecorator
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECompareStateMode Comparison = ECompareStateMode::Equals;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECombatState Towards = ECombatState::Passive;

    UFUNCTION(BlueprintCallable)
    bool PerformConditionCheckAI(class AAIController* OwnerController);
};
