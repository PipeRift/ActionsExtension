// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "GameplayTagsManager.h"

// Settings
#include "AIExtensionSettings.h"

DEFINE_LOG_CATEGORY(LogAIExtension)

#define LOCTEXT_NAMESPACE "AIExtensionModule"

FName FAIExtensionModule::FBehaviorTags::Combat   (TEXT("AI.Behavior.Combat"));
FName FAIExtensionModule::FBehaviorTags::Alert	(TEXT("AI.Behavior.Alert"));
FName FAIExtensionModule::FBehaviorTags::Suspicion(TEXT("AI.Behavior.Suspicion"));
FName FAIExtensionModule::FBehaviorTags::Passive  (TEXT("AI.Behavior.Passive"));

void FAIExtensionModule::StartupModule()
{
	UE_LOG(LogAIExtension, Warning, TEXT("AIExtension: Log Started"));

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	LoadGameplayTags();

	RegisterSettings();
}

void FAIExtensionModule::ShutdownModule()
{
	UE_LOG(LogAIExtension, Warning, TEXT("AIExtension: Log Ended"));
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (UObjectInitialized())
	{
		UnregisterSettings();
	}
}

void FAIExtensionModule::RegisterSettings()
{
#if WITH_EDITOR
	// Registering some settings is just a matter of exposing the default UObject of
	// your desired class, feel free to add here all those settings you want to expose
	// to your LDs or artists.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		// Get Project Settings category
		ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

		// Register AIExtension settings
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Game", "AIExtension",
			LOCTEXT("RuntimeAIExtensionSettingsName", "AIExtension"),
			LOCTEXT("RuntimeAIExtensionDescription", "AIExtension configuration of Jink core."),
			GetMutableDefault<UAIExtensionSettings>());

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FAIExtensionModule::HandleSettingsSaved);
		}
	}
#endif
}

void FAIExtensionModule::UnregisterSettings()
{
#if WITH_EDITOR
	// Ensure to unregister all of your registered settings here, hot-reload would
	// otherwise yield unexpected results.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Game", "AIExtension");
	}
#endif
}

void FAIExtensionModule::LoadGameplayTags()
{
	UGameplayTagsManager& GameplayTags = UGameplayTagsManager::Get();

	GameplayTags.AddNativeGameplayTag(FBehaviorTags::Passive);
	GameplayTags.AddNativeGameplayTag(FBehaviorTags::Suspicion);
	GameplayTags.AddNativeGameplayTag(FBehaviorTags::Alert);
	GameplayTags.AddNativeGameplayTag(FBehaviorTags::Combat);
}

bool FAIExtensionModule::HandleSettingsSaved()
{
	UAIExtensionSettings* Settings = GetMutableDefault<UAIExtensionSettings>();
	bool ResaveSettings = false;

	if (ModifiedSettingsDelegate.IsBound()) {
		ModifiedSettingsDelegate.Execute();
	}

	// You can put any validation code in here and resave the settings in case an invalid
	// value has been entered

	if (ResaveSettings)
	{
		Settings->SaveConfig();
	}
	
	return true;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAIExtensionModule, AIExtension)