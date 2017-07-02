// Copyright 2015-2017 Piperift. All Rights Reserved.

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
    UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Owner", DisplayName = "Create Task", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static UTask* CreateTask(const TScriptInterface<ITaskOwnerInterface>& Owner, const TSubclassOf<class UTask> TaskType);
    
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Activate", BlueprintInternalUseOnly = "true"), Category = BehaviourTree)
    static void ActivateTask(UTask* const Task);

};
