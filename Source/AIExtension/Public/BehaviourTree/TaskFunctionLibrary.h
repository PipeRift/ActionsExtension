// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPBT_Node.h"
#include "TaskFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UTaskFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", DisplayName = "Create Node", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static UBPBT_Node* CreateTask(UObject* WorldContextObject, TSubclassOf<class UBPBT_Node> TaskType);

};
