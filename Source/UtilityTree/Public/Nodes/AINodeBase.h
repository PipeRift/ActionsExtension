// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "AINodeBase.generated.h"

class UUtilityTreeBlueprint;
class UUtilityTree;
struct FUtilityTreeProxy;
struct FAINode_Base;


/**
* We pass array items by reference, which is scary as TArray can move items around in memory.
* So we make sure to allocate enough here so it doesn't happen and crash on us.
*/
#define AI_NODE_DEBUG_MAX_CHAIN 50
#define AI_NODE_DEBUG_MAX_CHILDREN 12
#define AI_NODE_DEBUG_MAX_CACHEPOSE 20

struct UTILITYTREE_API FAINodeDebugData
{
private:
	struct DebugItem
	{
		DebugItem(FString Data, bool bInPoseSource) : DebugData(Data), bPoseSource(bInPoseSource) {}

		/** This node item's debug text to display. */
		FString DebugData;

		/** Whether we are supplying a pose instead of modifying one (e.g. an playing animation). */
		bool bPoseSource;

		/** Nodes that we are connected to. */
		TArray<FAINodeDebugData> ChildNodeChain;
	};

	/** This nodes final contribution weight (based on its own weight and the weight of its parents). */
	float AbsoluteWeight;

	/** Nodes that we are dependent on. */
	TArray<DebugItem> NodeChain;

	/** Additional info provided, used in GetNodeName. States machines can provide the state names for the Root Nodes to use for example. */
	FString NodeDescription;

	/** Pointer to RootNode */
	FAINodeDebugData* RootNodePtr;

	/** SaveCachePose Nodes */
	TArray<FAINodeDebugData> SaveCachePoseNodes;

public:
	struct FFlattenedDebugData
	{
		FFlattenedDebugData(FString Line, float AbsWeight, int32 InIndent, int32 InChainID, bool bInPoseSource) : DebugLine(Line), AbsoluteWeight(AbsWeight), Indent(InIndent), ChainID(InChainID), bPoseSource(bInPoseSource) {}
		FString DebugLine;
		float AbsoluteWeight;
		int32 Indent;
		int32 ChainID;
		bool bPoseSource;

		bool IsOnActiveBranch() { return false;/*TODO*/ }
	};

	FAINodeDebugData(const class UUtilityTree* InUtilityTree)
		: AbsoluteWeight(1.f), RootNodePtr(this), UtilityTree(InUtilityTree)
	{
		SaveCachePoseNodes.Reserve(AI_NODE_DEBUG_MAX_CACHEPOSE);
	}

	FAINodeDebugData(const class UUtilityTree* InUtilityTree, const float AbsWeight, FString InNodeDescription, FAINodeDebugData* InRootNodePtr)
		: AbsoluteWeight(AbsWeight)
		, NodeDescription(InNodeDescription)
		, RootNodePtr(InRootNodePtr)
		, UtilityTree(InUtilityTree)
	{}

	void AddDebugItem(FString DebugData, bool bPoseSource = false);
	FAINodeDebugData& BranchFlow(float BranchWeight, FString InNodeDescription = FString());
	FAINodeDebugData* GetCachePoseDebugData(float GlobalWeight);

	template<class Type>
	FString GetNodeName(Type* Node)
	{
		FString FinalString = FString::Printf(TEXT("%s<W:%.1f%%> %s"), *Node->StaticStruct()->GetName(), AbsoluteWeight*100.f, *NodeDescription);
		NodeDescription.Empty();
		return FinalString;
	}

	void GetFlattenedDebugData(TArray<FFlattenedDebugData>& FlattenedDebugData, int32 Indent, int32& ChainID);

	TArray<FFlattenedDebugData> GetFlattenedDebugData()
	{
		TArray<FFlattenedDebugData> Data;
		int32 ChainID = 0;
		GetFlattenedDebugData(Data, 0, ChainID);
		return Data;
	}

	// Anim instance that we are generating debug data for
	const UUtilityTree* UtilityTree;
};



#define ENABLE_AIGRAPH_TRAVERSAL_DEBUG 0

/** A pose link to another node */
USTRUCT(BlueprintInternalUseOnly)
struct UTILITYTREE_API FAILinkBase
{
	GENERATED_USTRUCT_BODY()

	/** Serialized link ID, used to build the non-serialized pointer map. */
	UPROPERTY()
	int32 LinkID;

#if WITH_EDITORONLY_DATA
	/** The source link ID, used for debug visualization. */
	UPROPERTY()
	int32 SourceLinkID;
#endif

#if ENABLE_AIGRAPH_TRAVERSAL_DEBUG
	FGraphTraversalCounter InitializationCounter;
	FGraphTraversalCounter CachedBonesCounter;
	FGraphTraversalCounter UpdateCounter;
	FGraphTraversalCounter EvaluationCounter;
#endif

protected:
	/** The non serialized node pointer. */
	struct FAINode_Base* LinkedNode;

	/** Flag to prevent reentry when dealing with circular trees. */
	bool bProcessed;

public:
	FAILinkBase()
		: LinkID(INDEX_NONE)
#if WITH_EDITORONLY_DATA
		, SourceLinkID(INDEX_NONE)
#endif
		, LinkedNode(NULL)
		, bProcessed(false)
	{
	}

	// Interface

	//void Initialize(const FAnimationInitializeContext& Context);
	//void CacheBones(const FAnimationCacheBonesContext& Context);
	//void Update(const FAnimationUpdateContext& Context);
	void GatherDebugData(FAINodeDebugData& DebugData);

	/** Try to re-establish the linked node pointer. */
	//void AttemptRelink(const FAnimationBaseContext& Context);
	/** This only used by custom handlers, and it is advanced feature. */
	void SetLinkNode(struct FAINode_Base* NewLinkNode);
	/** This only used by custom handlers, and it is advanced feature. */
	FAINode_Base* GetLinkNode();
};

#define ENABLE_AINODE_POSE_DEBUG 0

/** A local-space pose link to another node */
USTRUCT(BlueprintInternalUseOnly)
struct UTILITYTREE_API FAILink : public FAILinkBase
{
	GENERATED_USTRUCT_BODY()

public:
	// Interface
	void Evaluate(FPoseContext& Output, bool bExpectsAdditivePose = false);

#if ENABLE_AINODE_POSE_DEBUG
private:
	// forwarded pose data from the wired node which current node's skeletal control is not applied yet
	FCompactHeapPose CurrentPose;
#endif //#if ENABLE_ANIMNODE_POSE_DEBUG
};



/**
 * This is the base of all runtime AI nodes
 *
 * To create a new AI node:
 *   Create a struct derived from FAINode_Base - this is your runtime node
 *   Create a class derived from UAnimGraphNode_Base, containing an instance of your runtime node as a member - this is your visual/editor-only node
 */
USTRUCT()
struct UTILITYTREE_API FAINode_Base
{
	GENERATED_USTRUCT_BODY()

	/** 
	 * Called when the node first runs. If the node is inside a state machine or cached pose branch then this can be called multiple times. 
	 * This can be called on any thread.
	 * @param	Context		Context structure providing access to relevant data
	 */
	//virtual void Initialize(const FAnimationInitializeContext& Context);

	/** 
	 * Called to update the state of the graph relative to this node.
	 * Generally this should configure any weights (etc.) that could affect the poses that
	 * will need to be evaluated. This function is what usually executes EvaluateGraphExposedInputs.
	 * This can be called on any thread.
	 * @param	Context		Context structure providing access to relevant data
	 */
	//virtual void Update(const FAnimationUpdateContext& Context);

	/** 
	 * Called to evaluate component-space bone transforms according to the weights set up in Update().
	 * You should implement either Evaluate or EvaluateComponentSpace, but not both of these.
	 * This can be called on any thread.
	 * @param	Output		Output structure to write pose or curve data to. Also provides access to relevant data as a context.
	 */	
	//virtual void EvaluateComponentSpace(FComponentSpacePoseContext& Output);

	/** 
	 * If a derived ai node should respond to asset overrides, OverrideAsset should be defined to handle changing the asset 
	 * This is called during ai blueprint compilation to handle child ai blueprints.
	 * @param	NewAsset	The new asset that is being set
	 */
	//virtual void OverrideAsset(UAnimationAsset* NewAsset) {}

	/**
	 * Called to gather on-screen debug data. 
	 * This is called on the game thread.
	 * @param	DebugData	Debug data structure used to output any relevant data
	 */
	virtual void GatherDebugData(FAINodeDebugData& DebugData)
	{ 
		DebugData.AddDebugItem(FString::Printf(TEXT("Non Overriden GatherDebugData! (%s)"), *DebugData.GetNodeName(this)));
	}

	/**
	 * Whether this node can run its Update() call on a worker thread.
	 * This is called on the game thread.
	 * If any node in a graph returns false from this function, then ALL nodes will update on the game thread.
	 */
	virtual bool CanUpdateInWorkerThread() const { return true; }

	/**
	 * Override this to indicate that PreUpdate() should be called on the game thread (usually to 
	 * This is called on the game thread.
	 * gather non-thread safe data) before Update() is called.
	 */
	virtual bool HasPreUpdate() const { return false; }

	/** Override this to perform game-thread work prior to non-game thread Update() being called */
	virtual void PreUpdate(const UUtilityTree* InUtilityTree) {}

	/**
	 * For nodes that implement some kind of simulation, return true here so ResetDynamics() gets called
	 * when things like teleports, time skips etc. occur that might require special handling
	 * This is called on the game thread.
	 */
	virtual bool NeedsDynamicReset() const { return false; }

	/** Override this to perform game-thread work prior to non-game thread Update() being called */
	virtual void ResetDynamics() {}

	/** Called after compilation */
	virtual void PostCompile() {}

	virtual ~FAINode_Base() {}

protected:

	/** Called once, from game thread as the parent utility tree is created */
	virtual void OnInitializeUtilityTree(/*const FAnimInstanceProxy* InProxy, */const UUtilityTree* InAnimInstance);

	friend struct FUtilityTreeProxy;
};