// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BrainComponent.h"
#include "GameplayTaskOwnerInterface.h"

#include "BPBehaviourTreeComponent.generated.h"

class UBPBT_Node;

/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API UBPBehaviourTreeComponent : public UBrainComponent, public IGameplayTaskOwnerInterface
{
    GENERATED_BODY()

public:
    UBPBehaviourTreeComponent(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintImplementableEvent, Category = BehaviourTree)
    void Root();

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", DisplayName = "Create Node", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    UBPBT_Node* Node(UObject* WorldContextObject, TSubclassOf<class UBPBT_Node> ItemType, APlayerController* OwningPlayer);

private:

    UPROPERTY()
    UBPBT_Node* LastExecutedNode;
};
