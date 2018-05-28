// Copyright 2016-2017 Frontwire Studios. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EnvQueryTest_IsInCombat.h"


#define LOCTEXT_NAMESPACE "UEnvQueryTest_IsInCombat"

UEnvQueryTest_IsInCombat::UEnvQueryTest_IsInCombat(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TestPurpose = EEnvTestPurpose::Filter;

	Cost = EEnvTestCost::Low;
	SetWorkOnFloatValues(false);

	BoolValue.DefaultValue = false;
}

void UEnvQueryTest_IsInCombat::RunAITest(AAIGeneric* AI, FEnvQueryInstance& QueryInstance) const
{
	UObject* Owner = QueryInstance.Owner.Get();
	if (!Owner)
		return;

	BoolValue.BindData(Owner, QueryInstance.QueryID);
	const bool bNegate = BoolValue.GetValue();

	const bool bIsInCombat = AI->IsInCombat();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		It.SetScore(TestPurpose, FilterType, bIsInCombat, !bNegate);
	}
}

FText UEnvQueryTest_IsInCombat::GetDescriptionDetails() const
{
	if (BoolValue.IsDynamic())
		return Super::GetDescriptionDetails();

	return BoolValue.GetValue()? LOCTEXT("IsNotInCombat", "Is in combat") : LOCTEXT("IsInCombat", "Is Not in combat");
}

#undef LOCTEXT_NAMESPACE
