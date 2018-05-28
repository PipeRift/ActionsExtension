// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EnvQueryContext_PotentialTargets.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "AISystem.h"
#include "VisualLogger/VisualLogger.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include "AIGeneric.h"


UEnvQueryContext_PotentialTargets::UEnvQueryContext_PotentialTargets(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UEnvQueryContext_PotentialTargets::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
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

	const TSet<APawn*>& PotentialTargets = AI->GetPotentialTargetsRef();
	
	TArray<AActor*> ActorTargets;

	ActorTargets.Reserve(PotentialTargets.Num());

	for (auto It = PotentialTargets.CreateConstIterator(); It; ++It)
	{
		ActorTargets.Add(*It);
	}

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, ActorTargets);
}



