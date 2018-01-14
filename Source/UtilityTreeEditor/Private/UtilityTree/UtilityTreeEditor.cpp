// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "UtilityTreeEditor.h"

#include "UtilityTreeEditorModule.h"

#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"

#if ENGINE_MINOR_VERSION >= 18
#include "HAL/PlatformApplicationMisc.h"
#endif

#include "SKismetInspector.h"

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

}

FUtilityTreeEditor::FUtilityTreeEditor()
{
	GEditor->OnBlueprintPreCompile().AddRaw(this, &FUtilityTreeEditor::OnBlueprintPreCompile);
}

FUtilityTreeEditor::~FUtilityTreeEditor()
{
	GEditor->OnBlueprintPreCompile().RemoveAll(this);

	FEditorDelegates::OnAssetPostImport.RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);
}

FName FUtilityTreeEditor::GetToolkitFName() const
{
	return FName("UtilityTreeEditor");
}

FText FUtilityTreeEditor::GetBaseToolkitName() const
{
	return LOCTEXT("UtilityTreeEditorAppLabel", "Utility Tree Editor");
}

FText FUtilityTreeEditor::GetToolkitName() const
{
	const TArray<UObject*>& EditingObjs = GetEditingObjects();

	check(EditingObjs.Num() > 0);

	FFormatNamedArguments Args;

	const UObject* EditingObject = EditingObjs[0];

	const bool bDirtyState = EditingObject->GetOutermost()->IsDirty();

	Args.Add(TEXT("ObjectName"), FText::FromString(EditingObject->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("UtilityTreeToolkitName", "{ObjectName}{DirtyState}"), Args);
}

FText FUtilityTreeEditor::GetToolkitToolTipText() const
{
	const UObject* EditingObject = GetEditingObject();

	check(EditingObject != NULL);

	return FAssetEditorToolkit::GetToolTipTextForObject(EditingObject);
}

FString FUtilityTreeEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("UtilityTreeEditor");
}

FLinearColor FUtilityTreeEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

UUtilityTreeBlueprint* FUtilityTreeEditor::GetUtilityTreeBlueprint() const
{
	return Cast<UUtilityTreeBlueprint>(GetBlueprintObj());
}

UBlueprint* FUtilityTreeEditor::GetBlueprintObj() const
{
	const TArray<UObject*>& EditingObjs = GetEditingObjects();
	for (int32 i = 0; i < EditingObjs.Num(); ++i)
	{
		if (EditingObjs[i]->IsA<UUtilityTreeBlueprint>())
		{
			return (UBlueprint*)EditingObjs[i];
		}
	}
	return nullptr;
}

void FUtilityTreeEditor::SetDetailObjects(const TArray<UObject*>& InObjects)
{
	Inspector->ShowDetailsForObjects(InObjects);
}

void FUtilityTreeEditor::SetDetailObject(UObject* Obj)
{
	TArray<UObject*> Objects;
	if (Obj)
	{
		Objects.Add(Obj);
	}
	SetDetailObjects(Objects);
}

void FUtilityTreeEditor::CreateDefaultCommands()
{
	if (GetBlueprintObj())
	{
		FBlueprintEditor::CreateDefaultCommands();
	}
	else
	{
		ToolkitCommands->MapAction(FGenericCommands::Get().Undo,
			FExecuteAction::CreateSP(this, &FUtilityTreeEditor::UndoAction));
		ToolkitCommands->MapAction(FGenericCommands::Get().Redo,
			FExecuteAction::CreateSP(this, &FUtilityTreeEditor::RedoAction));
	}
}

void FUtilityTreeEditor::OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList)
{
	//GraphEditorCommandsList->MapAction(FAIGraphCommands::Get().ToggleAIWatch,
		//FExecuteAction::CreateSP(this, &FUtilityTreeEditor::OnToggleAIWatch));
}

void FUtilityTreeEditor::Compile()
{
	// Grab the currently debugged object, so we can re-set it below
	UUtilityTree* CurrentDebugObject = nullptr;
	if (UBlueprint* Blueprint = GetBlueprintObj())
	{
		CurrentDebugObject = Cast<UUtilityTree>(Blueprint->GetObjectBeingDebugged());
		if (CurrentDebugObject)
		{
			// Force close any asset editors that are using the AnimScriptInstance (such as the Property Matrix), the class will be garbage collected
			FAssetEditorManager::Get().CloseOtherEditors(CurrentDebugObject, nullptr);
		}
	}

	// Compile the blueprint
	FBlueprintEditor::Compile();

	if (CurrentDebugObject != nullptr)
	{
		GetBlueprintObj()->SetObjectBeingDebugged(CurrentDebugObject);
	}

	// reset the selected skeletal control node
	SelectedAIGraphNode.Reset();

	// if the user manipulated Pin values directly from the node, then should copy updated values to the internal node to retain data consistency
	OnPostCompile();
}

/** Called when graph editor focus is changed */
void FUtilityTreeEditor::OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor)
{
	// in the future, depending on which graph editor is this will act different
	FBlueprintEditor::OnGraphEditorFocused(InGraphEditor);

	// install callback to allow us to propagate pin default changes live to the preview
	//UAIGraph* AIGraph = Cast<UAIGraph>(InGraphEditor->GetCurrentGraph());
	//if (AIGraph)
	//{
		//OnPinDefaultValueChangedHandle = AIGraph->OnPinDefaultValueChanged.Add(FOnPinDefaultValueChanged::FDelegate::CreateSP(this, &FUtilityTreeEditor::HandlePinDefaultValueChanged));
	//}
}

void FUtilityTreeEditor::OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	FBlueprintEditor::OnGraphEditorBackgrounded(InGraphEditor);

	/*UAIGraph* AIGraph = Cast<UAIGraph>(InGraphEditor->GetCurrentGraph());
	if (AIGraph)
	{
		AIGraph->OnPinDefaultValueChanged.Remove(OnPinDefaultValueChangedHandle);
	}*/
}

void FUtilityTreeEditor::PostUndo(bool bSuccess)
{
	DocumentManager->CleanInvalidTabs();
	DocumentManager->RefreshAllTabs();

	FBlueprintEditor::PostUndo(bSuccess);

	// If we undid a node creation that caused us to clean up a tab/graph we need to refresh the UI state
	RefreshEditors();

	// PostUndo broadcast
	OnPostUndo.Broadcast();

	OnPostCompile();
}

void FUtilityTreeEditor::PostRedo(bool bSuccess)
{
	DocumentManager->RefreshAllTabs();

	FBlueprintEditor::PostRedo(bSuccess);

	// PostUndo broadcast, OnPostRedo
	OnPostUndo.Broadcast();

	// calls PostCompile to copy proper values between AI nodes
	OnPostCompile();
}

void FUtilityTreeEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	FBlueprintEditor::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
}

void FUtilityTreeEditor::UndoAction()
{
	GEditor->UndoTransaction();
}

void FUtilityTreeEditor::RedoAction()
{
	GEditor->RedoTransaction();
}

void FUtilityTreeEditor::OnBlueprintPreCompile(UBlueprint* BlueprintToCompile)
{}

void FUtilityTreeEditor::OnPostCompile()
{}

#undef LOCTEXT_NAMESPACE
