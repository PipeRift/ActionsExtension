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

    UPROPERTY(BlueprintAssignable)
    FTaskDelayDelegate OnFinish;

    virtual void Activate() override;

    /** Return debug string describing task */
    virtual FString GetDebugString() const override;

    /** Wait specified time. This is functionally the same as a standard Delay node. */
    UFUNCTION(BlueprintCallable, Category = "GameplayTasks", meta = (AdvancedDisplay = "TaskOwner, Priority", DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE"))
    static UBPBT_Node* Node(UBPBehaviourTreeComponent* BT, float Time, const uint8 Priority = 192);

private:

    void OnTimeFinish();

    float Time;
    float TimeStarted;
};
