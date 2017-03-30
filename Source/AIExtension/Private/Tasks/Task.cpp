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

void AActor::BeginPlay()
{
    ensureMsgf(ActorHasBegunPlay == EActorBeginPlayState::HasNotBegunPlay, TEXT("BeginPlay was called on actor %s which was in state %d"), *GetPathName(), ActorHasBegunPlay);
    SetLifeSpan(InitialLifeSpan);
    RegisterAllActorTickFunctions(true, false); // Components are done below.

    TInlineComponentArray<UActorComponent*> Components;
    GetComponents(Components);

    ActorHasBegunPlay = EActorBeginPlayState::BeginningPlay;
    for (UActorComponent* Component : Components)
    {
        // bHasBegunPlay will be true for the component if the component was renamed and moved to a new outer during initialization
        if (Component->IsRegistered() && !Component->HasBegunPlay())
        {
            Component->RegisterAllComponentTickFunctions(true);
            Component->BeginPlay();
        }
        else
        {
            // When an Actor begins play we expect only the not bAutoRegister false components to not be registered
            //check(!Component->bAutoRegister);
        }
    }

    ReceiveBeginPlay();

    ActorHasBegunPlay = EActorBeginPlayState::HasBegunPlay;
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