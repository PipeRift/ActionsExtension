// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AIGeneric.h"
#include "BTD_OrderEquals.h"

bool UBTD_OrderEquals::PerformConditionCheckAI(AAIController * OwnerController)
{
    const auto AIGen = Cast<AAIGeneric>(OwnerController);
    if (!IsValid(AIGen))
    {
        return false;
    }

    // Not to sure this is how to correctly do class comparisons
    return Compare.Get() && AIGen->GetOrder() == Compare;
}
