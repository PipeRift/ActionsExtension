// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UtilityTree.generated.h"


#if WITH_EDITOR
class UUtilityTree;

/** Interface for quest group graph interaction with the AudioEditor module. */
//class IUtilityTreeEditorInfo
//{
//public:
//	virtual ~IUtilityTreeEditorInfo() {}
//
//	/** Called when creating a new quest group graph. */
//	virtual UEdGraph* CreateNewQuestGroupGraph(UUtilityTree* InQuest) = 0;
//
//	/** Sets up a quest node. */
//	//virtual void SetupNode(UEdGraph* QuestGraph, UQGNode* QuestNode, bool bSelectNewNode) = 0;
//
//	/** Links graph nodes from quest nodes. */
//	virtual void LinkGraphNodesFromQuestNodes(UUtilityTree* QuestGroup) = 0;
//
//	/** Compiles quest nodes from graph nodes. */
//	virtual void CompileQuestNodesFromGraphNodes(UUtilityTree* QuestGroup) = 0;
//
//	/** Removes nodes which are null from the quest group graph. */
//	virtual void RemoveNullNodes(UUtilityTree* QuestGroup) = 0;
//
//	/** Renames all pins in a quest group node */
//	//virtual void RenameNodePins(UQGNode* QGNode) = 0;
//};
#endif

/**
 * 
 */
UCLASS(Blueprintable, Category = AIExtension)
class UTILITYTREE_API UUtilityTree : public UObject
{
	GENERATED_BODY()
	
	
	
	
};
