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

	/**
	 * Selecting this option will cause the compiler to emit warnings whenever a call into Blueprint
	 * is made from the animation graph. This can help track down optimizations that need to be made.
	 */
	UPROPERTY(EditAnywhere, Category = Optimization)
	bool bWarnAboutBlueprintUsage;


#if WITH_EDITOR

	virtual UClass* GetBlueprintClass() const override;

	// Inspects the hierarchy and looks for an override for the requested node GUID
	// @param NodeGuid - Guid of the node to search for
	// @param bIgnoreSelf - Ignore this blueprint and only search parents, handy for finding parent overrides
	FUTParentNodeAssetOverride* GetAssetOverrideForNode(FGuid NodeGuid, bool bIgnoreSelf = false) const ;

	// Inspects the hierarchy and builds a list of all asset overrides for this blueprint
	// @param OutOverrides - Array to fill with overrides
	// @return bool - Whether any overrides were found
	bool GetAssetOverrides(TArray<FUTParentNodeAssetOverride*>& OutOverrides);

	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const override
	{
		return false;
	}

	virtual bool IsValidForBytecodeOnlyRecompile() const override { return false; }
	virtual bool CanRecompileWhilePlayingInEditor() const override;
	// End of UBlueprint interface

	/** Returns the most base utility blueprint for a given blueprint (if it is inherited from another utility blueprint, returning null if only native / non-utility BP classes are it's parent) */
	static UUtilityTreeBlueprint* FindRootUtilityBlueprint(UUtilityTreeBlueprint* DerivedBlueprint);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOverrideChangedMulticaster, FGuid, UUtilityTree*);

	typedef FOnOverrideChangedMulticaster::FDelegate FOnOverrideChanged;

	void RegisterOnOverrideChanged(const FOnOverrideChanged& Delegate)
	{
		OnOverrideChanged.Add(Delegate);
	}

	void UnregisterOnOverrideChanged(SWidget* Widget)
	{
		OnOverrideChanged.RemoveAll(Widget);
	}

	void NotifyOverrideChange(FUTParentNodeAssetOverride& Override)
	{
		OnOverrideChanged.Broadcast(Override.ParentNodeGuid, Override.NewAsset);
	}

	virtual void PostLoad() override;

	virtual void Serialize(FArchive& Ar) override;

protected:
	// Broadcast when an override is changed, allowing derived blueprints to be updated
	FOnOverrideChangedMulticaster OnOverrideChanged;
#endif	// #if WITH_EDITOR

#if WITH_EDITORONLY_DATA
public:
	// Array of overrides to asset containing nodes in the parent that have been overridden
	UPROPERTY()
	TArray<FUTParentNodeAssetOverride> ParentAssetOverrides;
#endif
};