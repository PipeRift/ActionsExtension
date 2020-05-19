// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>

#ifndef HAS_ACTIONS_ANALYTICS
	// Helps IDEs expect the definition to be true
	#define HAS_ACTIONS_ANALYTICS 1
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogActionsAnalytics, All, All)

class FActionsAnalytics : public IModuleInterface
{
public:

	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}

	static inline FActionsAnalytics& Get() {
		return FModuleManager::LoadModuleChecked<FActionsAnalytics>("ActionsAnalytics");
	}

	static inline bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("ActionsAnalytics");
	}
};
