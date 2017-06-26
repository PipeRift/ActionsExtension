// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AI_Generic.h"
#include "BTD_IsInSquad.h"

bool UBTD_IsInSquad::PerformConditionCheckAI(AAIController* OwnerController)
{
    auto AIGen = Cast<AAI_Generic>(OwnerController);

    if (!IsValid(AIGen))
    {
        return false;
    }

    return AIGen->IsInSquad();
}
