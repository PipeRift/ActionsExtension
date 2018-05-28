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
FUTParentNodeAssetOverride* UUtilityTreeBlueprint::GetAssetOverrideForNode(FGuid NodeGuid, bool bIgnoreSelf) const
{
	TArray<UBlueprint*> Hierarchy;
	GetBlueprintHierarchyFromClass(GetUTBlueprintGeneratedClass(), Hierarchy);

	for (int32 Idx = bIgnoreSelf ? 1 : 0; Idx < Hierarchy.Num(); ++Idx)
	{
		if (UUtilityTreeBlueprint* UTBlueprint = Cast<UUtilityTreeBlueprint>(Hierarchy[Idx]))
		{
			FUTParentNodeAssetOverride* Override = UTBlueprint->ParentAssetOverrides.FindByPredicate([NodeGuid](const FUTParentNodeAssetOverride& Other)
			{
				return Other.ParentNodeGuid == NodeGuid;
			});

			if (Override)
			{
				return Override;
			}
		}
	}

	return nullptr;
}

bool UUtilityTreeBlueprint::GetAssetOverrides(TArray<FUTParentNodeAssetOverride*>& OutOverrides)
{
	TArray<UBlueprint*> Hierarchy;
	GetBlueprintHierarchyFromClass(GetUTBlueprintGeneratedClass(), Hierarchy);

	for (UBlueprint* Blueprint : Hierarchy)
	{
		if (UUtilityTreeBlueprint* UTBlueprint = Cast<UUtilityTreeBlueprint>(Blueprint))
		{
			for (FUTParentNodeAssetOverride& Override : UTBlueprint->ParentAssetOverrides)
			{
				bool OverrideExists = OutOverrides.ContainsByPredicate([Override](const FUTParentNodeAssetOverride* Other)
				{
					return Override.ParentNodeGuid == Other->ParentNodeGuid;
				});

				if (!OverrideExists)
				{
					OutOverrides.Add(&Override);
				}
			}
		}
	}

	return OutOverrides.Num() > 0;
}

class UUTBlueprintGeneratedClass* UUtilityTreeBlueprint::GetUTBlueprintGeneratedClass() const
{
	return Cast<UUTBlueprintGeneratedClass>(*GeneratedClass);
}

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
