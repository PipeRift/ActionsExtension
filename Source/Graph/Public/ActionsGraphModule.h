// Copyright 2015-2023 Piperift. All Rights Reserved.
#pragma once

#include <Modules/ModuleManager.h>


DECLARE_LOG_CATEGORY_EXTERN(LogActionsGraph, All, All)

class FActionsGraphModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};