// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Task.h"
#include "TaskFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UTaskFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", DisplayName = "Create Task", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static UTask* CreateTask(UObject* WorldContextObject, TSubclassOf<class UTask> TaskType);
    
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "Owner", DisplayName = "Activate", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static void ActivateTask(UObject* Owner, UTask* Task);

};
