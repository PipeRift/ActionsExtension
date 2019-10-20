// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>


class FActionsTest : public IModuleInterface
{
public:

	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}

	static inline FActionsTest& Get() {
		return FModuleManager::LoadModuleChecked<FActionsTest>("ActionsTest");
	}

	static inline bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("ActionsTest");
	}
};

IMPLEMENT_MODULE(FActionsTest, ActionsTest);
