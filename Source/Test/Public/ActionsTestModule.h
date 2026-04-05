// Copyright 2015-2026 Piperift. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>


class FActionsTestModule : public IModuleInterface
{
public:
	void StartupModule() override;

	static inline FActionsTestModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FActionsTestModule>("ActionsTest");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ActionsTest");
	}
};

IMPLEMENT_MODULE(FActionsTestModule, ActionsTest);
