// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TickableObject.h"

UTickableObject::UTickableObject(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bWantsToTick = true;
    bTickInEditor = false;
    TickRate = 0;

    DeltaElapsed = 0;
}

void UTickableObject::BeginPlay()
{
    ensureMsgf(ObjectHasBegunPlay == EObjectBeginPlayState::HasNotBegunPlay, TEXT("BeginPlay was called on tickable object %s which was in state %d"), *GetPathName(), ObjectHasBegunPlay);

    ObjectHasBegunPlay = EObjectBeginPlayState::BeginningPlay;

    DeltaElapsed = 0;
    ReceiveBeginPlay();

    ObjectHasBegunPlay = EObjectBeginPlayState::HasBegunPlay;
}

void UTickableObject::Tick(float DeltaTime) {
    if (TickRate > 0) {
        DeltaElapsed += DeltaTime;
        if (DeltaElapsed < TickRate)
            return;

        DeltaElapsed -= TickRate;
    }

    if (!HasObjectBegunPlay() && !bTickInEditor)
        return;

    // Limited Tick

    //TODO: Elapsed value may be wrong. Check it.
    OTick(DeltaElapsed);
    ReceiveTick(DeltaElapsed);
}

void UTickableObject::OTick(float DeltaTime) {
}



void UTickableObject::DispatchBeginPlay() {
    if (!HasObjectBegunPlay() && !IsPendingKill()) {
        BeginPlay();
    }
}

void UTickableObject::PostInitProperties() {
    Super::PostInitProperties();

    if (GEngine) {
        UWorld* World = GEngine->GetWorld();
        if (World && World->WorldType != EWorldType::Editor) {
            DispatchBeginPlay();
        }
    }
}
