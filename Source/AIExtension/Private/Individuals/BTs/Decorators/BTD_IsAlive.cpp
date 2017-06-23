// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AI_Generic.h"
#include "BTD_IsAlive.h"

bool UBTD_IsAlive::PerformConditionCheckAI(AAIController* OwnerController)
{
    auto AIGen = Cast<AAI_Generic>(OwnerController);

    return IsValid(AIGen);
}
