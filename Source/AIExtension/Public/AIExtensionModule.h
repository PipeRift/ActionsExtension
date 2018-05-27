// Copyright 2015-2018 Piperift. All Rights Reserved.
#pragma once

#include "Private/AIExtensionPrivatePCH.h"

#include "AIModule.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"

#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#endif //WITH_EDITOR

DECLARE_LOG_CATEGORY_EXTERN(LogAIExtension, All, All);


class FAIExtensionModule : public IModuleInterface
{

public:

    // Get Jink Core module instance
    FORCEINLINE static FAIExtensionModule& GetInstance() { 
        return FModuleManager::LoadModuleChecked<FAIExtensionModule>("AIExtension");
    }

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    virtual bool SupportsDynamicReloading() override { return true; }

    DECLARE_DELEGATE_RetVal(void, FOnModifiedSettings)
    FOnModifiedSettings& OnModifiedSettings()
    {
        return ModifiedSettingsDelegate;
    }

private:
    /** Holds a delegate that is executed after the settings section has been modified. */
    FOnModifiedSettings ModifiedSettingsDelegate;

    void RegisterSettings();
    void UnregisterSettings();


    void LoadGameplayTags();

    // Callbacks for when the settings were saved.
    bool HandleSettingsSaved();

#if WITH_EDITOR
public:
    EAssetTypeCategories::Type GetAssetCategoryBit() const {
        return IAIModule::Get().GetAIAssetCategoryBit();
    }
#endif

public:
    struct FBehaviorTags
    {
        static FName Combat;
        static FName Alert;
        static FName Suspicion;
        static FName Passive;
    };
};
