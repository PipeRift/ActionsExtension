// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "GameplayTask.h"

#include "BPBehaviourTreeComponent.h"

#include "BPBT_Node.generated.h"

/**
 * Result of a node execution
 */
UENUM()
enum EBTNodeResult
{
    NR_RUNNING UMETA(DisplayName = "Running"),
    NR_SUCCESS UMETA(DisplayName = "Success"),
    NR_FAILURE UMETA(DisplayName = "Failure"),
    NR_ERROR   UMETA(DisplayName = "Error")
};

/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API UBPBT_Node : public UObject
{
    GENERATED_BODY()

public:
    UBPBT_Node(const FObjectInitializer& ObjectInitializer);

    virtual EBTNodeResult Activate();

protected:
    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Activate"))
    void ReceiveActivate();
};
