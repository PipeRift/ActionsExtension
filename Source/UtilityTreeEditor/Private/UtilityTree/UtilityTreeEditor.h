// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Toolkits/IToolkitHost.h"
#include "IUtilityTreeEditor.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"
#include "BlueprintEditor.h"

class IDetailsView;
class SDockableTab;
class SGraphEditor;
class SQuestPalette;
class UEdGraphNode;
class UUtilityTree;
struct FPropertyChangedEvent;
struct Rect;

/*-----------------------------------------------------------------------------
   FUtilityTreeEditor
-----------------------------------------------------------------------------*/

class FUtilityTreeEditor : public FBlueprintEditor, public IUtilityTreeEditor
{
public:
	/** Edits the specified Quest object */
	void InitUtilityTreeEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode);


public:
	FUtilityTreeEditor();

	virtual ~FUtilityTreeEditor();


	// IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	// End of IToolkit interface


	UUtilityTreeBlueprint* GetUtilityTreeBlueprint() const;

	/** Returns a pointer to the Blueprint object we are currently editing, as long as we are editing exactly one */
	virtual UBlueprint* GetBlueprintObj() const override;

	/** Update the inspector that displays information about the current selection*/
	void SetDetailObjects(const TArray<UObject*>& InObjects);
	void SetDetailObject(UObject* Obj);


protected:
	//~ Begin FBlueprintEditor Interface
	virtual void CreateDefaultCommands() override;
	virtual void OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList);
	virtual void Compile() override;
	virtual void OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor) override;
	virtual void OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor) override;
	//~ End FBlueprintEditor Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) override;
	//~ End FNotifyHook Interface


public:

	// Called after an undo is performed to give child widgets a chance to refresh
	typedef FSimpleMulticastDelegate::FDelegate FOnPostUndo;

	/** Registers a delegate to be called after an Undo operation */
	void RegisterOnPostUndo(const FOnPostUndo& Delegate)
	{
		OnPostUndo.Add(Delegate);
	}

	/** Unregisters a delegate to be called after an Undo operation */
	void UnregisterOnPostUndo(SWidget* Widget)
	{
		OnPostUndo.RemoveAll(Widget);
	}

	/** Delegate called after an undo operation for child widgets to refresh */
	FSimpleMulticastDelegate OnPostUndo;


protected:
	/** Undo Action**/
	void UndoAction();
	/** Redo Action **/
	void RedoAction();

private:

	/** Called immediately prior to a blueprint compilation */
	void OnBlueprintPreCompile(UBlueprint* BlueprintToCompile);

	/** Called post compile to copy node data */
	void OnPostCompile();


	// selected Utility Tree graph node 
	TWeakObjectPtr<class UAIGraphNode_Base> SelectedAIGraphNode;
};
