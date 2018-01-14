// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "SquadOrder.h"
#include "BTD_OrderEquals.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTD_OrderEquals : public UBTDecorator
{
	GENERATED_BODY()	
	
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Node")
    TSubclassOf<USquadOrder> Compare;

    UFUNCTION(BlueprintCallable)
    bool PerformConditionCheckAI(class AAIController* OwnerController);
};
