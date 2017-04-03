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
}

void UTask::BeginPlay()
{
    Super::BeginPlay();
}

void UTask::OTick(float DeltaTime) {
}

void UTask::Activate()
{
    if (IsActivated())
        return;

    State = ETaskState::RUNNING;
    
    OnActivation();
    //Freezes the game
    ReceiveActivate();
}

void UTask::ReceiveActivate_Implementation() {}

void UTask::FinishTask(bool bSuccess, bool bError) {
    if (!IsActivated())
        return;

    if (bError) {
        State = ETaskState::ERROR;
        return;
    }

    State = bSuccess ? ETaskState::SUCCESS : ETaskState::FAILURE;
    
    if(Succeeded()) {
        OnSuccess.Broadcast();
    } else {
        OnFailure.Broadcast();
    }

    Destroy();
}