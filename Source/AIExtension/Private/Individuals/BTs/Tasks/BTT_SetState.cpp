// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AI_Generic.h"
#include "BTT_SetState.h"

EBTNodeResult::Type UBTT_SetState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AController* Controller = Cast<AController>(OwnerComp.GetOwner());
    auto AIGen = Cast<AAI_Generic>(Controller);

    if (!IsValid(AIGen))
    {
        return EBTNodeResult::Failed;
    }

    AIGen->State = State;
    return EBTNodeResult::Succeeded;
}