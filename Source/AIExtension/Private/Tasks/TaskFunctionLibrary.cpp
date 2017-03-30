// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TaskFunctionLibrary.h"

UTask* UTaskFunctionLibrary::CreateTask(UObject* WorldContextObject, TSubclassOf<class UTask> TaskType)
{
    if (!TaskType->IsValidLowLevel() || TaskType == UTask::StaticClass())
        return nullptr;

    return NewObject<UTask>(WorldContextObject, TaskType);
}

void UTaskFunctionLibrary::ActivateTask(UObject* Owner, UTask* Task)
{
    if (Task) {
        Task->Activate();
    }
}


