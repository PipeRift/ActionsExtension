// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AIGeneric.h"
#include "BTT_SetState.h"

UBTT_SetState::UBTT_SetState()
{
    State = ECombatState::Passive;
}

EBTNodeResult::Type UBTT_SetState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    auto AIGen = Cast<AAIGeneric>(OwnerComp.GetOwner());

    if (!IsValid(AIGen))
    {
        return EBTNodeResult::Failed;
    }

    AIGen->State = State;
    return EBTNodeResult::Succeeded;
}