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
    UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Outer", DisplayName = "Create Task", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static UTask* CreateTask(UObject* Outer, TSubclassOf<class UTask> TaskType);
    
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Activate", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static void ActivateTask(UTask* Task);

};
