// Copyright 2015-2019 Piperift. All Rights Reserved.
#pragma once

#if WITH_EDITOR
#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#endif //WITH_EDITOR

DECLARE_LOG_CATEGORY_EXTERN(LogActions, All, All);


class FActionsModule : public IModuleInterface
{

public:

	// Get Jink Core module instance
	FORCEINLINE static FActionsModule& GetInstance() { 
		return FModuleManager::LoadModuleChecked<FActionsModule>("Actions");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override { return true; }

#if WITH_EDITOR
public:
	EAssetTypeCategories::Type GetAssetCategoryBit() const {
		return EAssetTypeCategories::Misc;
	}
#endif
};
