// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIGeneric.h"
#include "AIFunctionLibrary.h"
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

FString UBTT_SetState::GetStaticDescription() const
{
    const FString StateName = UAIFunctionLibrary::CombatStateToString(State);
    return FString::Printf(TEXT("State: %s"), *StateName);
}
