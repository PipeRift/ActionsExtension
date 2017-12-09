// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UtilityTreeBlueprint.h"
#include "UObject/FrameworkObjectVersion.h"

//////////////////////////////////////////////////////////////////////////
// UUtilityTreeBlueprint

UUtilityTreeBlueprint::UUtilityTreeBlueprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR

UUtilityTreeBlueprint* UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(UUtilityTreeBlueprint* DerivedBlueprint)
{
	UUtilityTreeBlueprint* ParentBP = NULL;

	// Determine if there is an utility tree blueprint in the ancestry of this class
	for (UClass* ParentClass = DerivedBlueprint->ParentClass; ParentClass && (UObject::StaticClass() != ParentClass); ParentClass = ParentClass->GetSuperClass())
	{
		if (UUtilityTreeBlueprint* TestBP = Cast<UUtilityTreeBlueprint>(ParentClass->ClassGeneratedBy))
		{
			ParentBP = TestBP;
		}
	}

	return ParentBP;
}

#endif
