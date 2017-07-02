// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "TaskFunctionLibrary.h"

UTask* UTaskFunctionLibrary::CreateTask(const TScriptInterface<ITaskOwnerInterface>& InOwner, const TSubclassOf<class UTask>& TaskType)
{
    if (!InOwner.GetObject())
        return nullptr;

    if (!TaskType->IsValidLowLevel() || TaskType == UTask::StaticClass())
        return nullptr;

    return NewObject<UTask>(InOwner.GetObject(), TaskType);
}

void UTaskFunctionLibrary::ActivateTask(UTask* const Task)
{
    if (Task) {
        Task->Activate();
    }
}


