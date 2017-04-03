// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TaskFunctionLibrary.h"

UTask* UTaskFunctionLibrary::CreateTask(UObject* Outer, TSubclassOf<class UTask> TaskType)
{
    if (!Outer)
        return nullptr;

    if (!TaskType->IsValidLowLevel() || TaskType == UTask::StaticClass())
        return nullptr;

    return NewObject<UTask>(Outer, TaskType);
}

void UTaskFunctionLibrary::ActivateTask(UTask* Task)
{
    if (Task) {
        Task->Activate();
    }
}


