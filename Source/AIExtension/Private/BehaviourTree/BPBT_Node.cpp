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

EBTNodeResult UBPBT_Node::Activate()
{
    UWorld* World = GetWorld();
    return ReceiveActivate();
}
