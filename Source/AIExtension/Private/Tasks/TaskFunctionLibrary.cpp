// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "TaskFunctionLibrary.h"

UTask* UTaskFunctionLibrary::CreateTask(const TScriptInterface<ITaskOwnerInterface>& Owner, const TSubclassOf<class UTask> TaskType)
{
    if (!Owner.GetObject())
        return nullptr;

    if (!TaskType->IsValidLowLevel() || TaskType == UTask::StaticClass())
        return nullptr;

    return NewObject<UTask>(Owner.GetObject(), TaskType);
}
