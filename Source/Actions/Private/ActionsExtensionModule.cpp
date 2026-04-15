// Copyright 2015-2026 Piperift. All Rights Reserved.

#include "ActionsExtensionModule.h"

#if WITH_GAMEPLAY_DEBUGGER
#	include "GameplayDebugger.h"
#	include "GameplayDebugger_Actions.h"
#endif	  // WITH_GAMEPLAY_DEBUGGER


DEFINE_LOG_CATEGORY(LogActions)

#define LOCTEXT_NAMESPACE "ActionsModule"

void FActionsExtensionModule::StartupModule()
{
	// Register Gameplay debugger
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory("Actions",
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebugger_Actions::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FActionsExtensionModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FActionsExtensionModule, ActionsExtension)