// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "BPBT_Node.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameplayTaskOwnerInterface.h"

UBPBT_Node::UBPBT_Node(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

UBPBT_Node* UBPBT_Node::Node(UBPBehaviourTreeComponent* BT, float Time, const uint8 Priority)
{
    UBPBT_Node* MyTask = NewTaskUninitialized<UBPBT_Node>();
    if (MyTask && Cast<IGameplayTaskOwnerInterface>(BT) != nullptr)
    {
        MyTask->InitTask(*BT, Priority);
        //Setup variables
    }
    return MyTask;
}

void UBPBT_Node::Activate()
{
    UWorld* World = GetWorld();
}
