// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AISquad.h"
#include "AIFunctionLibrary.h"

#include "BTD_CompareState.h"

UBTD_CompareState::UBTD_CompareState()
	: Super()
{
	NodeName = TEXT("Compare State");
	Comparison = ECompareStateMode::Equals;
	State = ECombatState::Passive;
}

bool UBTD_CompareState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIGeneric* AIOwner = Cast<AAIGeneric>(OwnerComp.GetAIOwner());
	if (!IsValid(AIOwner))
		return false;

	auto CurrentState = AIOwner->State;

	switch (Comparison)
	{
	case ECompareStateMode::Equals:
		return CurrentState == State;
	case ECompareStateMode::Greater:
		return CurrentState > State;
	case ECompareStateMode::Less:
		return CurrentState < State;
	case ECompareStateMode::GreaterOrEqual:
		return CurrentState >= State;
	case ECompareStateMode::LessOrEqual:
		return CurrentState <= State;
	case ECompareStateMode::NotEqual:
		return CurrentState != State;
	default:
		return false;
	}
}

FString UBTD_CompareState::GetStaticDescription() const
{
	const FString ComparisonName = UAIFunctionLibrary::CompareStateModeToString(Comparison);
	const FString StateName = UAIFunctionLibrary::CombatStateToString(State);
	return FString::Printf(TEXT("State %s %s"), *ComparisonName, *StateName);
}
