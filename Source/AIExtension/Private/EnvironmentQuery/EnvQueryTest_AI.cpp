// Copyright 2016-2017 Frontwire Studios. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EnvQueryTest_AI.h"


UEnvQueryTest_AI::UEnvQueryTest_AI(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UEnvQueryTest_AI::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* Owner = QueryInstance.Owner.Get();

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

	RunAITest(AI, QueryInstance);
}

void UEnvQueryTest_AI::RunAITest(AAIGeneric* AI, FEnvQueryInstance& QueryInstance) const
{

}
