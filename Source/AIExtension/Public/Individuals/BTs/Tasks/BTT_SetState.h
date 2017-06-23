// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EAIEnums.h"
#include "BTT_SetState.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTT_SetState : public UBTTaskNode
{
	GENERATED_BODY()	
	
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECombatState State = ECombatState::EPassive;

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
