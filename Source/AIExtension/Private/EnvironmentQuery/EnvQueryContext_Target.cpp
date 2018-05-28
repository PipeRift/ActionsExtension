// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EnvQueryContext_Target.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "AISystem.h"
#include "VisualLogger/VisualLogger.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include "AIGeneric.h"


UEnvQueryContext_Target::UEnvQueryContext_Target(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UEnvQueryContext_Target::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* Owner = Cast<AActor>(QueryInstance.Owner.Get());

	AAIGeneric* AI = Cast<AAIGeneric>(Owner);
	if (!AI)
	{
		APawn* OwnerAsPawn = Cast<APawn>(Owner);
		if (!OwnerAsPawn)
			return;

		AI = Cast<AAIGeneric>(OwnerAsPawn->GetController());
	}

	if (!AI)
		return;

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, AI->GetTarget());
}


