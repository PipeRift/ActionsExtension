// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"

#include "Task_BehaviourTree.h"

UTask_BehaviourTree::UTask_BehaviourTree(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bWantsToTick = true;
    TickRate = 0.15f;

    BTState = EBTState::NOT_RUN;
}

void UTask_BehaviourTree::OnActivation() {
}

void UTask_BehaviourTree::TaskTick(float DeltaTime) {
    Super::TaskTick(DeltaTime);

    if (!IsRunning()) {
        SetState(EBTState::RUNNING);
        Root();
    }
}

void UTask_BehaviourTree::Success() {
    SetState(EBTState::SUCCESS);
}

void UTask_BehaviourTree::Failure(bool bError) {
    SetState(bError? EBTState::ERROR : EBTState::FAILURE);
}

void UTask_BehaviourTree::SetState(EBTState NewState) {
    BTState = NewState;
}

