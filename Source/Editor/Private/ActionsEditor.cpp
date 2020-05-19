// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "ActionsEditor.h"
#include <Kismet2/KismetEditorUtilities.h>
#if HAS_ACTIONS_ANALYTICS
#include <ActionsEOSPlatform.h>
#endif

#include "Action.h"


#define LOCTEXT_NAMESPACE "FActionsEditorModule"

DEFINE_LOG_CATEGORY(LogActionsEd)

void FActionsEditorModule::StartupModule()
{
	UE_LOG(LogActionsEd, Log, TEXT("Actions Editor: Log Started"));

	RegisterPropertyTypeCustomizations();
	PrepareAutoGeneratedDefaultEvents();

#if HAS_ACTIONS_ANALYTICS
	if(FActionsEOSPlatform::Get().Create())
	{
		FActionsEOSPlatform::Get().GetMetrics()->BeginSession();
	}
#endif
}

void FActionsEditorModule::ShutdownModule()
{
	UE_LOG(LogActionsEd, Log, TEXT("Actions Editor: Log Ended"));

#if HAS_ACTIONS_ANALYTICS
	FActionsEOSPlatform::Get().Release();
#endif

	CreatedAssetTypeActions.Empty();

	// Cleanup all information for auto generated default event nodes by this module
	FKismetEditorUtilities::UnregisterAutoBlueprintNodeCreation(this);
}


void FActionsEditorModule::RegisterPropertyTypeCustomizations()
{
}

void FActionsEditorModule::PrepareAutoGeneratedDefaultEvents()
{
	//Task events
	RegisterDefaultEvent(UAction, ReceiveActivate);
	RegisterDefaultEvent(UAction, ReceiveTick);
	RegisterDefaultEvent(UAction, ReceiveFinished);
}


void FActionsEditorModule::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate)
{
	check(PropertyTypeName != NAME_None);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FActionsEditorModule, ActionsEditor);