// Copyright 2015-2026 Piperift. All Rights Reserved.

#include "ActionsTestModule.h"
#include "Automatron.h"



void FActionsTestModule::StartupModule()
{
	Automatron::RegisterSpecs();
}
