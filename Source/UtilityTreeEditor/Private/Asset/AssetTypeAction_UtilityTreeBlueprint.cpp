// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AssetTypeAction_UtilityTreeBlueprint.h"

#include "AIExtensionModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "BlueprintEditorUtils.h"

#include "UtilityTree.h"
#include "UtilityTreeBlueprint.h"

#include "UtilityTreeBlueprintFactory.h"
#include "UtilityTree/UtilityTreeEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_UtilityTreeBlueprint

FText FAssetTypeAction_UtilityTreeBlueprint::GetName() const
{
    return LOCTEXT("FAssetTypeAction_UtilityTreeBlueprintName", "Utility Tree Blueprint");
}

FColor FAssetTypeAction_UtilityTreeBlueprint::GetTypeColor() const
{
    return FColor(202, 65, 244);
}

UClass* FAssetTypeAction_UtilityTreeBlueprint::GetSupportedClass() const
{
    return UUtilityTreeBlueprint::StaticClass();
}

void FAssetTypeAction_UtilityTreeBlueprint::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
}

uint32 FAssetTypeAction_UtilityTreeBlueprint::GetCategories()
{
    return FAIExtensionModule::GetInstance().GetAssetCategoryBit();
}

void FAssetTypeAction_UtilityTreeBlueprint::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Blueprint = Cast<UBlueprint>(*ObjIt);
		if (Blueprint && Blueprint->SkeletonGeneratedClass && Blueprint->GeneratedClass)
		{
			TSharedRef< FUtilityTreeEditor > NewEditor(new FUtilityTreeEditor());

			TArray<UBlueprint*> Blueprints;
			Blueprints.Add(Blueprint);

			NewEditor->InitUtilityTreeEditor(Mode, EditWithinLevelEditor, Blueprints, ShouldUseDataOnlyEditor(Blueprint));
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToLoadAbilityBlueprint", "Utility Tree Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

UFactory* FAssetTypeAction_UtilityTreeBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UUtilityTreeBlueprintFactory* UTBlueprintFactory = NewObject<UUtilityTreeBlueprintFactory>();
	//UUtilityTreeBlueprint* UTBlueprint = CastChecked<UUtilityTreeBlueprint>(InBlueprint);
	UTBlueprintFactory->ParentClass = TSubclassOf<UUtilityTree>(*InBlueprint->GeneratedClass);
	return UTBlueprintFactory;
}

bool FAssetTypeAction_UtilityTreeBlueprint::ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const
{
	return FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint)
		&& !FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint)
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint)
		&& !Blueprint->bForceFullEditor
		&& !Blueprint->bIsNewlyCreated;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
