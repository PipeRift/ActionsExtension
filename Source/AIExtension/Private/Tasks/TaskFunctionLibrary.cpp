// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "TaskFunctionLibrary.h"

UTask* UTaskFunctionLibrary::CreateTask(const TScriptInterface<ITaskOwnerInterface>& Owner, const TSubclassOf<class UTask> TaskType, bool bAutoActivate/* = false*/)
{
    if (!Owner.GetObject())
        return nullptr;

    if (!TaskType.Get() || TaskType == UTask::StaticClass())
        return nullptr;

    if (!bAutoActivate)
    {
        return NewObject<UTask>(Owner.GetObject(), TaskType);
    }
    else
    {
        UTask* Task = NewObject<UTask>(Owner.GetObject(), TaskType);
        Task->Activate();
        return Task;
    }
}
