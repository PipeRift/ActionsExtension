// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "Task.h"

#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameplayTaskOwnerInterface.h"

UTask::UTask(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    State = ETaskState::NOT_RUN;
    bWantsToTick = true;
    TickRate = 0;

    Elapsed = 0;
}


void UTask::Tick(float DeltaTime) {
    if (TickRate > 0) {
        Elapsed += DeltaTime;
        if (Elapsed < TickRate) return;

        Elapsed -= TickRate;
    }

    // Limited Tick

    //TODO: Elapsed value may be wrong. Check it.
    TaskTick(Elapsed);
    ReceiveTick(Elapsed);
}

void UTask::TaskTick(float DeltaTime) {
}

void UTask::Activate()
{
    if (IsActivated())
        return;

    State = ETaskState::RUNNING;
    
    OnActivation();
    ReceiveActivate();
}

void UTask::FinishTask(bool bSuccess, bool bError) {
    if (bError) {
        State = ETaskState::ERROR;
        return;
    }

    State = bSuccess ? ETaskState::SUCCESS : ETaskState::FAILURE;
}