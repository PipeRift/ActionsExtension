// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Engine/Blueprint.h"
#include "UtilityTree.h"

#include "UtilityTreeBlueprint.generated.h"

class SWidget;
class UAnimationAsset;
class USkeletalMesh;


USTRUCT()
struct FUTParentNodeAssetOverride
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UUtilityTree* NewAsset;
	UPROPERTY()
	FGuid ParentNodeGuid;

	FUTParentNodeAssetOverride(FGuid InGuid, UUtilityTree* InNewAsset)
		: NewAsset(InNewAsset)
		, ParentNodeGuid(InGuid)
	{}

	FUTParentNodeAssetOverride()
		: NewAsset(NULL)
	{}

	bool operator ==(const FUTParentNodeAssetOverride& Other)
	{
		return ParentNodeGuid == Other.ParentNodeGuid;
	}
};

/**
 * An Anim Blueprint is essentially a specialized Blueprint whose graphs control the animation of a Skeletal Mesh.
 * It can perform blending of animations, directly control the bones of the skeleton, and output a final pose
 * for a Skeletal Mesh each frame.
 */
UCLASS(BlueprintType)
class UTILITYTREE_API UUtilityTreeBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR

	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const override
	{
		return false;
	}
	// End of UBlueprint interface

	/** Returns the most base utility tree blueprint for a given blueprint (if it is inherited from another ability blueprint, returning null if only native / non-ability BP classes are it's parent) */
	static UUtilityTreeBlueprint* FindRootUtilityTreeBlueprint(UUtilityTreeBlueprint* DerivedBlueprint);

#endif
};