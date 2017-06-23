// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AI_Generic.h"
#include "BTD_OrderEquals.h"

bool UBTD_OrderEquals::PerformConditionCheckAI(AAIController * OwnerController)
{
    auto AIGen = Cast<AAI_Generic>(OwnerController);
    if (!IsValid(AIGen))
    {
        return false;
    }

    // Not to sure this is how to correctly do class comparisons
    return AIGen->GetOrder() == Compare;
}
