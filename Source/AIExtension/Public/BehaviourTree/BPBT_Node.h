// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "GameplayTask.h"

#include "BPBehaviourTreeComponent.h"

#include "BPBT_Node.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API UBPBT_Node : public UGameplayTask
{
    GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTaskDelayDelegate);

public:
    UBPBT_Node(const FObjectInitializer& ObjectInitializer);

    virtual void Activate() override;

protected:
    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Activate"))
    void ReceiveActivate();

    /** Wait specified time. This is functionally the same as a standard Delay node. */
    UFUNCTION(BlueprintCallable, Category = "BehaviourTree", meta = (AdvancedDisplay = "BT, Priority", DefaultToSelf = "BT", BlueprintInternalUseOnly = "TRUE"))
    static UBPBT_Node* Node(UBPBehaviourTreeComponent* BT, float Time, const uint8 Priority = 192);
};
