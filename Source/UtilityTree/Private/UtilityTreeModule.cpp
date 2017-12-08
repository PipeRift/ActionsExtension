// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "UtilityTreeModule.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#endif // WITH_GAMEPLAY_DEBUGGER


DEFINE_LOG_CATEGORY(LogUtility)

#define LOCTEXT_NAMESPACE "UtilityTreeModule"

void FUtilityTreeModule::StartupModule()
{
    UE_LOG(LogUtility, Warning, TEXT("UtilityTree: Log Started"));

    RegisterSettings();
}

void FUtilityTreeModule::ShutdownModule()
{
    UE_LOG(LogUtility, Warning, TEXT("UtilityTree: Log Ended"));
    
    if (UObjectInitialized())
    {
        UnregisterSettings();
    }
}

void FUtilityTreeModule::RegisterSettings()
{
}

void FUtilityTreeModule::UnregisterSettings()
{
}

bool FUtilityTreeModule::HandleSettingsSaved()
{
    return true;
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FUtilityTreeModule, UtilityTree)