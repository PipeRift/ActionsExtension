// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_SetState.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTT_SetState : public UBTTaskNode
{
	GENERATED_BODY()	
	
public:
    UBTT_SetState();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node)
    ECombatState State;

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    virtual FString GetStaticDescription() const override;
};
