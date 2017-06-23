// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_IsInSquad.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTD_IsInSquad : public UBTDecorator
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable)
    bool PerformConditionCheckAI(AAIController* OwnerController);
};
