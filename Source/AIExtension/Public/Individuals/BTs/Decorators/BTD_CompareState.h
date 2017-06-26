// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "EAIEnums.h"
#include "AI_Generic.h"
#include "BTD_CompareState.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECompareStateMode : uint8
{
    EEquals         UMETA(DisplayName = "Equals"),
    EGreaterThan    UMETA(DisplayName = "Greater Than"),
    ELessThan       UMETA(DisplayName = "Less Than"),
    EGreaterOrEqual UMETA(DisplayName = "Greater Than or Equal to"),
    ELessOrEqual    UMETA(DisplayName = "Less Than or Equal to"),
    ENotEqual       UMETA(DisplayName = "Not Equal")
};

UCLASS()
class AIEXTENSION_API UBTD_CompareState : public UBTDecorator
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECompareStateMode Comparison = ECompareStateMode::EEquals;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECombatState Towards = ECombatState::EPassive;

    UFUNCTION(BlueprintCallable)
    bool PerformConditionCheckAI(class AAIController* OwnerController);
};
