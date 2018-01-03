// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Nodes/AINodeBase.h"
//#include "UtilityTreeProxy.h"


void FAILinkBase::GatherDebugData(FAINodeDebugData& DebugData)
{
	if (LinkedNode != NULL)
	{
		LinkedNode->GatherDebugData(DebugData);
	}
}


/////////////////////////////////////////////////////
// FAINode_Base

/*void FAINode_Base::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	EvaluateGraphExposedInputs.Initialize(this, Context.AnimInstanceProxy->GetAnimInstanceObject());
}*/

//void FAINode_Base::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {}

//void FAINode_Base::Update_AnyThread(const FAnimationUpdateContext& Context) {}

//void FAINode_Base::Evaluate_AnyThread(FPoseContext& Output) {}

//void FAINode_Base::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) {}

void FAINode_Base::OnInitializeUtilityTree(/*const FUtilityTreeProxy* InProxy, */const UUtilityTree* InUtilityTree)
{}

/////////////////////////////////////////////////////
// FNodeDebugData

void FAINodeDebugData::AddDebugItem(FString DebugData, bool bPoseSource)
{
	check(NodeChain.Num() == 0 || NodeChain.Last().ChildNodeChain.Num() == 0); //Cannot add to this chain once we have branched

	NodeChain.Add( DebugItem(DebugData, bPoseSource) );
	NodeChain.Last().ChildNodeChain.Reserve(AI_NODE_DEBUG_MAX_CHILDREN);
}

FAINodeDebugData& FAINodeDebugData::BranchFlow(float BranchWeight, FString InNodeDescription)
{
	NodeChain.Last().ChildNodeChain.Add(FAINodeDebugData(UtilityTree, BranchWeight*AbsoluteWeight, InNodeDescription, RootNodePtr));
	NodeChain.Last().ChildNodeChain.Last().NodeChain.Reserve(AI_NODE_DEBUG_MAX_CHAIN);
	return NodeChain.Last().ChildNodeChain.Last();
}

FAINodeDebugData* FAINodeDebugData::GetCachePoseDebugData(float GlobalWeight)
{
	check(RootNodePtr);

	RootNodePtr->SaveCachePoseNodes.Add( FAINodeDebugData(UtilityTree, GlobalWeight, FString(), RootNodePtr) );
	RootNodePtr->SaveCachePoseNodes.Last().NodeChain.Reserve(AI_NODE_DEBUG_MAX_CHAIN);
	return &RootNodePtr->SaveCachePoseNodes.Last();
}

void FAINodeDebugData::GetFlattenedDebugData(TArray<FFlattenedDebugData>& FlattenedDebugData, int32 Indent, int32& ChainID)
{
	int32 CurrChainID = ChainID;
	for(DebugItem& Item : NodeChain)
	{
		FlattenedDebugData.Add( FFlattenedDebugData(Item.DebugData, AbsoluteWeight, Indent, CurrChainID, Item.bPoseSource) );
		bool bMultiBranch = Item.ChildNodeChain.Num() > 1;
		int32 ChildIndent = bMultiBranch ? Indent + 1 : Indent;
		for(FAINodeDebugData& Child : Item.ChildNodeChain)
		{
			if(bMultiBranch)
			{
				// If we only have one branch we treat it as the same really
				// as we may have only changed active status
				++ChainID;
			}
			Child.GetFlattenedDebugData(FlattenedDebugData, ChildIndent, ChainID);
		}
	}

	// Do CachePose nodes only from the root.
	if (RootNodePtr == this)
	{
		for (FAINodeDebugData& CachePoseData : SaveCachePoseNodes)
		{
			++ChainID;
			CachePoseData.GetFlattenedDebugData(FlattenedDebugData, 0, ChainID);
		}
	}
}
