// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneEventSection.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AINotify.h"
#include "BTTask_Notify.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTTask_Notify : public UBTTaskNode
{
	GENERATED_BODY()

    UBTTask_Notify();


    /** The name of the event to trigger */
    UPROPERTY(EditAnywhere, Category = Notify)
    FName EventName;

    /** The event parameters */
    UPROPERTY(EditAnywhere, Category = Notify, meta = (ShowOnlyInnerProperties))
    FMovieSceneEventParameters Parameters;


    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    virtual FString GetStaticDescription() const override;

};
