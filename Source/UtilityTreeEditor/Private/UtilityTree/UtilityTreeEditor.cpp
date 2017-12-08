// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "UtilityTreeEditor.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"

#if ENGINE_MINOR_VERSION >= 18
#include "HAL/PlatformApplicationMisc.h"
#endif

#include "ScopedTransaction.h"
//#include "GraphEditor.h"
//#include "GraphEditorActions.h"
//#include "EdGraph/EdGraphNode.h"
//#include "Kismet2/BlueprintEditorUtils.h"
//#include "EdGraphUtilities.h"
//#include "SNodePanel.h"
//#include "PropertyEditorModule.h"
//#include "IDetailsView.h"
//#include "Widgets/Docking/SDockTab.h"
//#include "Framework/Commands/GenericCommands.h"
//#include "Framework/MultiBox/MultiBoxBuilder.h"

//#include "QuestGraph/UtilityTreeGraph.h"
//#include "QuestGraph/UtilityTreeNode.h"
//#include "QuestGraph/UtilityTreeNode_Base.h"
//#include "QuestGraph/UtilityTreeNode_Root.h"
//#include "QuestGraph/UtilityTreeGraphSchema.h"

#include "UtilityTree.h"

//#include "UtilityTreeEditorCommands.h"

#define LOCTEXT_NAMESPACE "UtilityTreeEditor"


const FName FUtilityTreeEditor::GraphCanvasTabId( TEXT( "UtilityTreeEditor_GraphCanvas" ) );
const FName FUtilityTreeEditor::PropertiesTabId( TEXT( "UtilityTreeEditor_Properties" ) );

FUtilityTreeEditor::FUtilityTreeEditor()
	: Quest(nullptr)
{
}

void FUtilityTreeEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_UtilityTreeEditor", "Utility Tree Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner( GraphCanvasTabId, FOnSpawnTab::CreateSP(this, &FUtilityTreeEditor::SpawnTab_GraphCanvas) )
		.SetDisplayName( LOCTEXT("GraphCanvasTab", "Viewport") )
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner( PropertiesTabId, FOnSpawnTab::CreateSP(this, &FUtilityTreeEditor::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT("DetailsTab", "Details") )
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FUtilityTreeEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner( GraphCanvasTabId );
	InTabManager->UnregisterTabSpawner( PropertiesTabId );
}

FUtilityTreeEditor::~FUtilityTreeEditor()
{
	GEditor->UnregisterForUndo( this );
}

void FUtilityTreeEditor::InitUtilityTreeEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
	UtilityTree = CastChecked<UUtilityTree>(ObjectToEdit);

	// Support undo/redo
	UtilityTree->SetFlags(RF_Transactional);
	
	GEditor->RegisterForUndo(this);

	FGraphEditorCommands::Register();
	FQuestGraphEditorCommands::Register();

	BindGraphCommands();

	CreateInternalWidgets();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_UtilityTreeEditor_Layout_v1")
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->AddTab(GetToolbarTabId(), ETabState::OpenedTab) ->SetHideTabWell( true )
		)
		->Split
		(
			FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal) ->SetSizeCoefficient(0.9f)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.225f)
				->AddTab(PropertiesTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.65f)
				->AddTab(GraphCanvasTabId, ETabState::OpenedTab) ->SetHideTabWell( true )
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, TEXT("UtilityTreeEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit, false);

	ExtendToolbar();
	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*if(IsWorldCentricAssetEditor())
	{
		SpawnToolkitTab(GetToolbarTabId(), FString(), EToolkitTabSpot::ToolBar);
		SpawnToolkitTab(GraphCanvasTabId, FString(), EToolkitTabSpot::Viewport);
		SpawnToolkitTab(PropertiesTabId, FString(), EToolkitTabSpot::Details);
	}*/
}

UUtilityTree* FUtilityTreeEditor::GetUtilityTree() const
{
	return UtilityTree;
}

void FUtilityTreeEditor::SetSelection(TArray<UObject*> SelectedObjects)
{
	if (TreeProperties.IsValid())
	{
		TreeProperties->SetObjects(SelectedObjects);
	}
}

/*bool FUtilityTreeEditor::GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding )
{
	return QuestGraphEditor->GetBoundsForSelectedNodes(Rect, Padding);
}*/

int32 FUtilityTreeEditor::GetNumberOfSelectedNodes() const
{
	return QuestGraphEditor->GetSelectedNodes().Num();
}

FName FUtilityTreeEditor::GetToolkitFName() const
{
	return FName("QuestEditor");
}

FText FUtilityTreeEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "QUest Editor");
}

FString FUtilityTreeEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Quest ").ToString();
}

FLinearColor FUtilityTreeEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

/*TSharedRef<SDockTab> FUtilityTreeEditor::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == GraphCanvasTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("QuestGraphCanvasTitle", "Viewport"));

	if (QuestGraphEditor.IsValid())
	{
		SpawnedTab->SetContent(QuestGraphEditor.ToSharedRef());
	}

	return SpawnedTab;
}*/

TSharedRef<SDockTab> FUtilityTreeEditor::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == PropertiesTabId );

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("UtilityTreeDetailsTitle", "Tree Details"))
		[
			TreeProperties.ToSharedRef()
		];
}

void FUtilityTreeEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject( UtilityTree );
}

void FUtilityTreeEditor::PostUndo(bool bSuccess)
{
	/*if (QuestGraphEditor.IsValid())
	{
		QuestGraphEditor->ClearSelectionSet();
		QuestGraphEditor->NotifyGraphChanged();
	}*/
}

void FUtilityTreeEditor::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class UProperty* PropertyThatChanged)
{
	/*if (QuestGraphEditor.IsValid() && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		QuestGraphEditor->NotifyGraphChanged();
	}*/
}

void FUtilityTreeEditor::CreateInternalWidgets()
{
	//QuestGraphEditor = CreateGraphEditorWidget();

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;
#if ENGINE_MINOR_VERSION >= 18
	Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
#else
	Args.DefaultsOnlyVisibility = FDetailsViewArgs::EEditDefaultsOnlyNodeVisibility::Hide;
#endif


	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TreeProperties = PropertyModule.CreateDetailView(Args);
	TreeProperties->SetObject( UtilityTree );
}

void FUtilityTreeEditor::ExtendToolbar()
{
	struct Local
{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Toolbar");
			{
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar )
		);

	AddToolbarExtender(ToolbarExtender);
}

void FUtilityTreeEditor::BindGraphCommands()
{
	/*const FQuestGraphEditorCommands& Commands = FQuestGraphEditorCommands::Get();

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP( this, &FUtilityTreeEditor::UndoGraphAction ));

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP( this, &FUtilityTreeEditor::RedoGraphAction ));*/
}

void FUtilityTreeEditor::SyncInBrowser()
{
	TArray<UObject*> ObjectsToSync;
	//const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	//for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	//{
		/*UUtilityTreeNode_Base* SelectedNode = Cast<UUtilityTreeNode_Base>(*NodeIt);

		if (SelectedNode)
		{
            //Set custom node behaviors
            //Ref: SoundCueEditor.cpp
		}*/
	//}

	if (ObjectsToSync.Num() > 0)
	{
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}
}

bool FUtilityTreeEditor::CanSyncInBrowser() const
{
	/*const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UUtilityTreeNode_Base* SelectedNode = Cast<UUtilityTreeNode_Base>(*NodeIt);

		if (SelectedNode)
        {
            //Check custom node behaviors
            //Ref: SoundCueEditor.cpp
		}
	}*/
	return false;
}

void FUtilityTreeEditor::OnCreateComment()
{
	//FUtilityTreeGraphSchemaAction_NewComment CommentAction;
	//CommentAction.PerformAction(Quest->QuestGraph, NULL, QuestGraphEditor->GetPasteLocation());
}

/*TSharedRef<SGraphEditor> FUtilityTreeEditor::CreateGraphEditorWidget()
{
	if ( !GraphEditorCommands.IsValid() )
	{
		GraphEditorCommands = MakeShareable( new FUICommandList );

		GraphEditorCommands->MapAction( FQuestGraphEditorCommands::Get().BrowserSync,
			FExecuteAction::CreateSP(this, &FUtilityTreeEditor::SyncInBrowser),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanSyncInBrowser ));

		// Graph Editor Commands
		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().CreateComment,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::OnCreateComment )
			);

		// Editing commands
		GraphEditorCommands->MapAction( FGenericCommands::Get().SelectAll,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::SelectAllNodes ),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanSelectAllNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::DeleteSelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanDeleteNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::CopySelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanCopyNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::CutSelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanCutNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::PasteNodes ),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanPasteNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Duplicate,
			FExecuteAction::CreateSP( this, &FUtilityTreeEditor::DuplicateNodes ),
			FCanExecuteAction::CreateSP( this, &FUtilityTreeEditor::CanDuplicateNodes )
			);
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Quest", "QUEST");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FUtilityTreeEditor::OnSelectedNodesChanged);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FUtilityTreeEditor::OnNodeTitleCommitted);
	//InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FUtilityTreeEditor::OpenQuest);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(Quest->GetGraph())
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}*/

FGraphPanelSelectionSet FUtilityTreeEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	/*if (TreeGraphEditor.IsValid())
	{
		CurrentSelection = QuestGraphEditor->GetSelectedNodes();
	}*/
	return CurrentSelection;
}

void FUtilityTreeEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	if(NewSelection.Num())
	{
		/*for(TSet<class UObject*>::TConstIterator SetIt(NewSelection);SetIt;++SetIt)
        {
            UUtilityTreeNode* GraphNode = Cast<UUtilityTreeNode>(*SetIt);

			if (!*SetIt || (*SetIt)->IsA<UUtilityTreeNode_Root>())
			{
				Selection.Add(GetQuest());
			}
            else if (GraphNode)
            {
                UQGNode* Node = GraphNode->QuestNode;
                if (!Node)
                {
                    Selection.Add(GraphNode);
                }
                else if (Node->IsA<UQGNode_Finish>())
                {
                    Selection.Add(GetQuest());
                }
                else if (Node->IsA<UQGNode_Logic>())
                {
                    Selection.Add(Node);
                }
                else
                {
                    Selection.Add(Node);
                }
            }
			else
			{
				Selection.Add(*SetIt);
			}
		}*/
	}
	else
	{
		Selection.Add(GetUtilityTree());
	}

	SetSelection(Selection);
}

void FUtilityTreeEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction( LOCTEXT( "RenameNode", "Rename Node" ) );
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void FUtilityTreeEditor::SelectAllNodes()
{
	//TreeGraphEditor->SelectAllNodes();
}

bool FUtilityTreeEditor::CanSelectAllNodes() const
{
	return true;
}

void FUtilityTreeEditor::DeleteSelectedNodes()
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "QuestEditorDeleteSelectedNode", "Delete Selected Quest Node") );

	QuestGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	QuestGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);

		if (Node->CanUserDeleteNode())
		{
			if (UUtilityTreeNode* UtilityTreeNode = Cast<UUtilityTreeNode>(Node))
			{
				UQGNode* DelNode = UtilityTreeNode->QuestNode;

				FBlueprintEditorUtils::RemoveNode(NULL, UtilityTreeNode, true);

				// Make sure UtilityTree is updated to match graph
				Quest->CompileQuestNodesFromGraphNodes();

				// Remove this node from the Quest Group's list of all Quest Nodes
				Quest->AllNodes.Remove(DelNode);
				Quest->MarkPackageDirty();
			}
			else
			{
				FBlueprintEditorUtils::RemoveNode(NULL, Node, true);
			}
		}
	}
}

bool FUtilityTreeEditor::CanDeleteNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (SelectedNodes.Num() == 1)
	{
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (Cast<UUtilityTreeNode_Root>(*NodeIt))
			{
				// Return false if only root node is selected, as it can't be deleted
				return false;
			}
		}
	}

	return SelectedNodes.Num() > 0;
}

void FUtilityTreeEditor::DeleteSelectedDuplicatableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	QuestGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != NULL) && Node->CanDuplicateNode())
		{
			QuestGraphEditor->SetNodeSelection(Node, true);
		}
		else
		{
			RemainingNodes.Add(Node);
		}
	}

	// Delete the duplicable nodes
	DeleteSelectedNodes();

	// Reselect whatever's left from the original selection after the deletion
	QuestGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(RemainingNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			QuestGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FUtilityTreeEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicatableNodes();
}

bool FUtilityTreeEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FUtilityTreeEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if(UUtilityTreeNode_Base* Node = Cast<UUtilityTreeNode_Base>(*SelectedIter))
		{
			Node->PrepareForCopying();
		}
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);

#if ENGINE_MINOR_VERSION >= 18
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
#else
	FPlatformMisc::ClipboardCopy(*ExportedText);
#endif

	// Make sure Quest remains the owner of the copied nodes
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UUtilityTreeNode* Node = Cast<UUtilityTreeNode>(*SelectedIter))
		{
			Node->PostCopyNode();
		}
	}
}

bool FUtilityTreeEditor::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != NULL) && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FUtilityTreeEditor::PasteNodes()
{
	PasteNodesHere(QuestGraphEditor->GetPasteLocation());
}

void FUtilityTreeEditor::PasteNodesHere(const FVector2D& Location)
{
	// Undo/Redo support
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "QuestEditorPaste", "Paste Quest Node") );
	Quest->GetGraph()->Modify();
	Quest->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	QuestGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;

#if ENGINE_MINOR_VERSION >= 18
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);
#else
	FPlatformMisc::ClipboardPaste(TextToImport);
#endif

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(Quest->GetGraph(), TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f,0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if ( PastedNodes.Num() > 0 )
	{
		float InvNumNodes = 1.0f/float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;

		if (UUtilityTreeNode* QGNode = Cast<UUtilityTreeNode>(Node))
		{
			//Quest->AllNodes.Add(QGNode->QGNode);
		}

		// Select the newly pasted stuff
		QuestGraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X ;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y ;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();
	}

	// Force new pasted SoundNodes to have same connections as graph nodes
	Quest->CompileQuestNodesFromGraphNodes();

	// Update UI
	QuestGraphEditor->NotifyGraphChanged();

	Quest->PostEditChange();
	Quest->MarkPackageDirty();
}

bool FUtilityTreeEditor::CanPasteNodes() const
{
	FString ClipboardContent;
#if ENGINE_MINOR_VERSION >= 18
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
#else
	FPlatformMisc::ClipboardPaste(ClipboardContent);
#endif

	return FEdGraphUtilities::CanImportNodesFromText(Quest->QuestGraph, ClipboardContent);
}

void FUtilityTreeEditor::DuplicateNodes()
{
	// Copy and paste current selection
	CopySelectedNodes();
	PasteNodes();
}

bool FUtilityTreeEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FUtilityTreeEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void FUtilityTreeEditor::RedoGraphAction()
{
	// Clear selection, to avoid holding refs to nodes that go away
	QuestGraphEditor->ClearSelectionSet();

	GEditor->RedoTransaction();
}

#undef LOCTEXT_NAMESPACE
