// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTD_IsAlive.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTD_IsAlive : public UBTDecorator
{
    GENERATED_BODY()    
    
public:
    UFUNCTION(BlueprintCallable)
        bool PerformConditionCheckAI(AAIController* OwnerController);
};
