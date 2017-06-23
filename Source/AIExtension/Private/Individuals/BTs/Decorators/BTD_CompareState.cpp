// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "BTD_CompareState.h"

bool UBTD_CompareState::PerformConditionCheckAI(AAIController* OwnerController)
{
    auto AIGen = Cast<AAI_Generic>(OwnerController);
    if (!IsValid(AIGen))
    {
        return false;
    }

    auto State = AIGen->State;

    switch (Comparison)
    {
        case ECompareStateMode::EEquals:
            return State == Towards;
        case ECompareStateMode::EGreaterThan:
            return State > Towards;
        case ECompareStateMode::ELessThan:
            return State < Towards;
        case ECompareStateMode::EGreaterOrEqual:
            return State >= Towards;
        case ECompareStateMode::ELessOrEqual:
            return State <= Towards;
        case ECompareStateMode::ENotEqual:
            return State != Towards;
    }

    // We'll never reach this code, but it won't compile without it :V
    // Thanks, MSVC/C++ spec
    return false;
}
