// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AIGeneric.h"
#include "BTD_IsInSquad.h"

bool UBTD_IsInSquad::PerformConditionCheckAI(AAIController* OwnerController)
{
	const auto AIGen = Cast<AAIGeneric>(OwnerController);
	return IsValid(AIGen) && AIGen->IsInSquad();
}
