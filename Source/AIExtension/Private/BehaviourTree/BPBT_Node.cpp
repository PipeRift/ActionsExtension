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
    Time = 0.f;
    TimeStarted = 0.f;
}

UBPBT_Node* UBPBT_Node::Node(UBPBehaviourTreeComponent* BT, float Time, const uint8 Priority)
{
    UBPBT_Node* MyTask = NewTaskUninitialized<UBPBT_Node>();
    if (MyTask && Cast<IGameplayTaskOwnerInterface>(BT) != nullptr)
    {
        MyTask->InitTask(*BT, Priority);
        MyTask->Time = Time;
    }
    return MyTask;
}

void UBPBT_Node::Activate()
{
    UWorld* World = GetWorld();
    TimeStarted = World->GetTimeSeconds();

    // Use a dummy timer handle as we don't need to store it for later but we don't need to look for something to clear
    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(TimerHandle, this, &UBPBT_Node::OnTimeFinish, Time, false);
}

void UBPBT_Node::OnTimeFinish()
{
    OnFinish.Broadcast();
    EndTask();
}

FString UBPBT_Node::GetDebugString() const
{
    float TimeLeft = Time - GetWorld()->TimeSince(TimeStarted);
    return FString::Printf(TEXT("WaitDelay. Time: %.2f. TimeLeft: %.2f"), Time, TimeLeft);
}


