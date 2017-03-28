// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TaskFunctionLibrary.h"


UBPBT_Node* UTaskFunctionLibrary::CreateTask(UObject* WorldContextObject, TSubclassOf<class UBPBT_Node> TaskType)
{
    if (!TaskType->IsValidLowLevel() || TaskType == UBPBT_Node::StaticClass())
        return nullptr;

    return NewObject<UBPBT_Node>(WorldContextObject, TaskType);
}


