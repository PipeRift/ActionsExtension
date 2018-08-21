// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ActionsModule.h"
#include "ActionsSettings.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "Actions/GameplayDebugger_Actions.h"
#endif // WITH_GAMEPLAY_DEBUGGER

DEFINE_LOG_CATEGORY(LogActions)

#define LOCTEXT_NAMESPACE "ActionsModule"

void FActionsModule::StartupModule()
{
	UE_LOG(LogActions, Log, TEXT("ActionsExtension: Log Started"));

	RegisterSettings();

	// Register Gameplay debugger
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory("Actions", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebugger_Actions::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FActionsModule::ShutdownModule()
{
	UE_LOG(LogActions, Log, TEXT("ActionsExtension: Log Ended"));

	if (UObjectInitialized())
	{
		UnregisterSettings();
	}
}

void FActionsModule::RegisterSettings()
{
#if WITH_EDITOR
	// Registering some settings is just a matter of exposing the default UObject of
	// your desired class, feel free to add here all those settings you want to expose
	// to your LDs or artists.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		// Get Project Settings category
		ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

		// Register Actions settings
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Game", "Actions",
			LOCTEXT("RuntimeActionsSettingsName", "Actions"),
			LOCTEXT("RuntimeActionsDescription", "Actions configuration of Jink core."),
			GetMutableDefault<UActionsSettings>());

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FActionsModule::HandleSettingsSaved);
		}
	}
#endif
}

void FActionsModule::UnregisterSettings()
{
#if WITH_EDITOR
	// Ensure to unregister all of your registered settings here, hot-reload would
	// otherwise yield unexpected results.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Game", "Actions");
	}
#endif
}

bool FActionsModule::HandleSettingsSaved()
{
	UActionsSettings* Settings = GetMutableDefault<UActionsSettings>();
	bool ResaveSettings = false;

	if (ModifiedSettingsDelegate.IsBound()) {
		ModifiedSettingsDelegate.Execute();
	}

	// You can put any validation code in here and re-save the settings in case an invalid
	// value has been entered

	if (ResaveSettings)
	{
		Settings->SaveConfig();
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FActionsModule, Actions)