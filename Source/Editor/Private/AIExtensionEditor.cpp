// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionEditorPrivatePCH.h"

DEFINE_LOG_CATEGORY(LogAIExtensionEditor)
 
#define LOCTEXT_NAMESPACE "AIExtensionEditor"
 
void FAIExtensionEditorModule::StartupModule()
{
    UE_LOG(LogAIExtensionEditor, Warning, TEXT("AIExtensionEditor: Log Started"));

    RegisterPropertyTypeCustomizations();

    // Register asset types
    //IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();  
    //RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeAction_Something));

    // Integrate JinkCore actions into existing editor context menus
    /*if (!IsRunningCommandlet())
    {
        FAIEContentBrowserExtensions::InstallHooks();
    }*/
}
 
void FAIExtensionEditorModule::ShutdownModule()
{
    UE_LOG(LogAIExtensionEditor, Warning, TEXT("AIExtensionEditor: Log Ended"));

    // Unregister all the asset types
    /*if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
        for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
        {
            AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
        }
    }*/
    CreatedAssetTypeActions.Empty();


    if (UObjectInitialized())
    {
        //FAIEContentBrowserExtensions::RemoveHooks();
    }
}


void FAIExtensionEditorModule::RegisterPropertyTypeCustomizations()
{
    //RegisterCustomPropertyTypeLayout("Something", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSomethingCustomization::MakeInstance));
}


void FAIExtensionEditorModule::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate)
{
    check(PropertyTypeName != NAME_None);

    static FName PropertyEditor("PropertyEditor");
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
    PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FAIExtensionEditorModule, AIExtensionEditor);