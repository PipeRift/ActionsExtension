// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TickableObject.h"

UTickableObject::UTickableObject(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bWantsToTick = true;
    TickRate = 0;

    Elapsed = 0;
}

void UTickableObject::BeginPlay()
{
    ensureMsgf(ActorHasBegunPlay == EObjectBeginPlayState::HasNotBegunPlay, TEXT("BeginPlay was called on tickable object %s which was in state %d"), *GetPathName(), ObjectHasBegunPlay);
    SetLifeSpan(InitialLifeSpan);
    RegisterAllActorTickFunctions(true, false); // Components are done below.

    ActorHasBegunPlay = EActorBeginPlayState::BeginningPlay;

    ReceiveBeginPlay();

    ActorHasBegunPlay = EActorBeginPlayState::HasBegunPlay;
}

void UTickeableObject::Tick(float DeltaTime) {
    if (TickRate > 0) {
        Elapsed += DeltaTime;
        if (Elapsed < TickRate) return;

        Elapsed -= TickRate;
    }

    // Limited Tick

    //TODO: Elapsed value may be wrong. Check it.
    TickeableObjectTick(Elapsed);
    ReceiveTick(Elapsed);
}

void UTickeableObject::Tick(float DeltaTime) {

}

void UTickeableObject::DispatchBeginPlay() {

}

