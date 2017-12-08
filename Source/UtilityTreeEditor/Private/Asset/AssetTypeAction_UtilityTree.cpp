// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AssetTypeAction_UtilityTree.h"

#include "AIExtensionModule.h"

#include "UtilityTree.h"
#include "UtilityTreeEditorModule.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_UtilityTree

FText FAssetTypeAction_UtilityTree::GetName() const
{
    return LOCTEXT("FAssetTypeAction_UtilityTreeName", "Utility Tree");
}

FColor FAssetTypeAction_UtilityTree::GetTypeColor() const
{
    return FColor(202, 65, 244);
}

UClass* FAssetTypeAction_UtilityTree::GetSupportedClass() const
{
    return UUtilityTree::StaticClass();
}

void FAssetTypeAction_UtilityTree::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
}

uint32 FAssetTypeAction_UtilityTree::GetCategories()
{
    return FAIExtensionModule::GetInstance().GetAssetCategoryBit();
}

void FAssetTypeAction_UtilityTree::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
    {
        auto UtilityTree = Cast<UUtilityTree>(*ObjIt);
        if (UtilityTree != NULL)
        {
            FUtilityTreeEditorModule* EditorModule = &FModuleManager::LoadModuleChecked<FUtilityTreeEditorModule>("UtilityTreeEditor");
            EditorModule->CreateUtilityTreeEditor(Mode, EditWithinLevelEditor, UtilityTree);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
