// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"

class UTILITYTREEEDITOR_API FAssetTypeAction_UtilityTreeBlueprint : public FAssetTypeActions_Blueprint
{
public:
    // IAssetTypeActions interface
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
    virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
    virtual uint32 GetCategories() override;
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
    // End of IAssetTypeActions interface

	// FAssetTypeActions_Blueprint interface
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;

private:
	/** Returns true if the blueprint is data only */
	bool ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const;
};
