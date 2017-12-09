// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "UtilityTreeEditor.h"

#include "UtilityTreeEditorModule.h"

#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"

#if ENGINE_MINOR_VERSION >= 18
#include "HAL/PlatformApplicationMisc.h"
#endif

#include "ScopedTransaction.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
//#include "EdGraph/EdGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphUtilities.h"
//#include "SNodePanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

//#include "QuestGraph/UtilityTreeGraph.h"
//#include "QuestGraph/UtilityTreeNode.h"
//#include "QuestGraph/UtilityTreeNode_Base.h"
//#include "QuestGraph/UtilityTreeNode_Root.h"
//#include "QuestGraph/UtilityTreeGraphSchema.h"

#include "UtilityTree.h"

//#include "UtilityTreeEditorCommands.h"

#define LOCTEXT_NAMESPACE "UtilityTreeEditor"

void FUtilityTreeEditor::InitUtilityTreeEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints,	bool bShouldOpenInDefaultsMode)
{
	InitBlueprintEditor(Mode, InitToolkitHost, InBlueprints, bShouldOpenInDefaultsMode);

	for (auto Blueprint : InBlueprints)
	{
		EnsureBlueprintIsUpToDate(Blueprint);
	}
}

void FUtilityTreeEditor::EnsureBlueprintIsUpToDate(UBlueprint* Blueprint)
{
#if WITH_EDITORONLY_DATA
	int32 Count = Blueprint->UbergraphPages.Num();
	for (auto Graph : Blueprint->UbergraphPages)
	{
		// remove the default event graph, if it exists, from existing Gameplay Ability Blueprints
		if (Graph->GetName() == "EventGraph" && Graph->Nodes.Num() == 0)
		{
			//check(!Graph->Schema->GetClass()->IsChildOf(UUtilityTreeGraphSchema::StaticClass()));
			//FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph);
			break;
		}
	}
#endif
}
#undef LOCTEXT_NAMESPACE
