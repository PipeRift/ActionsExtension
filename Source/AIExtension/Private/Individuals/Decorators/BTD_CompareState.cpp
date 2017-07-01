// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AISquad.h"
#include "BTD_CompareState.h"

bool UBTD_CompareState::PerformConditionCheckAI(AAIController* OwnerController)
{
    const auto AIGen = Cast<AAIGeneric>(OwnerController);
    if (!IsValid(AIGen))
    {
        return false;
    }

    auto State = AIGen->State;

    switch (Comparison)
    {
        case ECompareStateMode::Equals:
            return State == Towards;
        case ECompareStateMode::GreaterThan:
            return State > Towards;
        case ECompareStateMode::LessThan:
            return State < Towards;
        case ECompareStateMode::GreaterOrEqual:
            return State >= Towards;
        case ECompareStateMode::LessOrEqual:
            return State <= Towards;
        case ECompareStateMode::NotEqual:
            return State != Towards;
        default:
            return false;
    }
}
