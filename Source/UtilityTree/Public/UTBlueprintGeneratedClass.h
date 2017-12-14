// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Engine/PoseWatch.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Animation/AnimClassInterface.h"

#include "UTBlueprintGeneratedClass.generated.h"

class UUTGraphNode_Base;
class UUtilityTree;
class UEdGraph;

// This structure represents debugging information for a frame snapshot
USTRUCT()
struct FUtilityTreeFrameSnapshot
{
	GENERATED_USTRUCT_BODY()

	FUtilityTreeFrameSnapshot()
#if WITH_EDITORONLY_DATA
		: TimeStamp(0.0)
#endif
	{
	}
#if WITH_EDITORONLY_DATA
public:
	// The snapshot of data saved from the animation
	TArray<uint8> SerializedData;

	// The time stamp for when this snapshot was taken (relative to the life timer of the object being recorded)
	double TimeStamp;

public:
	void InitializeFromInstance(UUtilityTree* Instance);
	ENGINE_API void CopyToInstance(UUtilityTree* Instance);
#endif
};

// This structure represents animation-related debugging information for an entire UtilityTreeBlueprint
// (general debug information for the event graph, etc... is still contained in a FBlueprintDebugData structure)
USTRUCT()
struct ENGINE_API FUtilityTreeBlueprintDebugData
{
	GENERATED_USTRUCT_BODY()

	FUtilityTreeBlueprintDebugData()
#if WITH_EDITORONLY_DATA
		: SnapshotBuffer(NULL)
		, SnapshotIndex(INDEX_NONE)
#endif
	{
	}

#if WITH_EDITORONLY_DATA
public:
	// Map from state machine graphs to their corresponding debug data
	TMap<TWeakObjectPtr<UEdGraph>, FStateMachineDebugData> StateMachineDebugData;

	// Map from state graphs to their node
	TMap<TWeakObjectPtr<UEdGraph>, TWeakObjectPtr<class UAnimStateNode> > StateGraphToNodeMap;

	// Map from transition graphs to their node
	TMap<TWeakObjectPtr<UEdGraph>, TWeakObjectPtr<class UAnimStateTransitionNode> > TransitionGraphToNodeMap;

	// Map from custom transition blend graphs to their node
	TMap<TWeakObjectPtr<UEdGraph>, TWeakObjectPtr<class UAnimStateTransitionNode> > TransitionBlendGraphToNodeMap;

	// Map from animation node to their property index
	TMap<TWeakObjectPtr<class UAnimGraphNode_Base>, int32> NodePropertyToIndexMap;

	// Map from animation node GUID to property index
	TMap<FGuid, int32> NodeGuidToIndexMap;

	// History of snapshots of animation data
	TSimpleRingBuffer<FAnimationFrameSnapshot>* SnapshotBuffer;

	// Node visit structure
	struct FNodeVisit
	{
		int32 SourceID;
		int32 TargetID;
		float Weight;

		FNodeVisit(int32 InSourceID, int32 InTargetID, float InWeight)
			: SourceID(InSourceID)
			, TargetID(InTargetID)
			, Weight(InWeight)
		{
		}
	};

	// History of activated nodes
	TArray<FNodeVisit> UpdatedNodesThisFrame;

	// Active pose watches to track
	TArray<FAnimNodePoseWatch> AnimNodePoseWatch;

	// Index of snapshot
	int32 SnapshotIndex;
public:

	~FUtilityTreeBlueprintDebugData()
	{
		if (SnapshotBuffer != NULL)
		{
			delete SnapshotBuffer;
		}
		SnapshotBuffer = NULL;
	}



	bool IsReplayingSnapshot() const { return SnapshotIndex != INDEX_NONE; }
	void TakeSnapshot(UAnimInstance* Instance);
	float GetSnapshotLengthInSeconds();
	int32 GetSnapshotLengthInFrames();
	void SetSnapshotIndexByTime(UAnimInstance* Instance, double TargetTime);
	void SetSnapshotIndex(UAnimInstance* Instance, int32 NewIndex);
	void ResetSnapshotBuffer();

	void ResetNodeVisitSites();
	void RecordNodeVisit(int32 TargetNodeIndex, int32 SourceNodeIndex, float BlendWeight);
	void RecordNodeVisitArray(const TArray<FNodeVisit>& Nodes);

	void AddPoseWatch(int32 NodeID, FColor Color);
	void RemovePoseWatch(int32 NodeID);
	void UpdatePoseWatchColour(int32 NodeID, FColor Color);
#endif
};

#if WITH_EDITORONLY_DATA
namespace EUTPropertySearchMode
{
	enum Type
	{
		OnlyThis,
		Hierarchy
	};
}
#endif

UCLASS()
class UTILITYTREE_API UUTBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

	// The index of the root node in the animation tree
	UPROPERTY()
	int32 RootAnimNodeIndex;

	// Indices for each of the saved pose nodes that require updating, in the order they need to get updates.
	UPROPERTY()
	TArray<int32> OrderedSavedPoseIndices;

	// The array of anim nodes; this is transient generated data (created during Link)
	UStructProperty* RootUTNodeProperty;
	TArray<UStructProperty*> UTNodeProperties;

public:

	virtual int32 GetRootUTNodeIndex() const override { return RootAnimNodeIndex; }

	virtual UStructProperty* GetRootUTNodeProperty() const override { return RootUTNodeProperty; }

	virtual const TArray<UStructProperty*>& GetUTNodeProperties() const override { return UTNodeProperties; }

	virtual const TArray<int32>& GetOrderedSavedPoseNodeIndices() const override { return OrderedSavedPoseIndices; }

public:
#if WITH_EDITORONLY_DATA
	FUTBlueprintDebugData UTBlueprintDebugData;

	FUTBlueprintDebugData& GetUTBlueprintDebugData()
	{
		return UTBlueprintDebugData;
	}

	template<typename StructType>
	const int32* GetNodePropertyIndexFromHierarchy(class UUTGraphNode_Base* Node)
	{
		TArray<const UBlueprintGeneratedClass*> BlueprintHierarchy;
		GetGeneratedClassesHierarchy(this, BlueprintHierarchy);

		for (const UBlueprintGeneratedClass* Blueprint : BlueprintHierarchy)
		{
			if (const UUTBlueprintGeneratedClass* UtilityTreeBlueprintClass = Cast<UUTBlueprintGeneratedClass>(Blueprint))
			{
				const int32* SearchIndex = UtilityTreeBlueprintClass->UTBlueprintDebugData.NodePropertyToIndexMap.Find(Node);
				if (SearchIndex)
				{
					return SearchIndex;
				}
			}

		}
		return NULL;
	}

	template<typename StructType>
	const int32* GetNodePropertyIndex(class UAnimGraphNode_Base* Node, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis)
	{
		return (SearchMode == EUTPropertySearchMode::OnlyThis) ? UTBlueprintDebugData.NodePropertyToIndexMap.Find(Node) : GetNodePropertyIndexFromHierarchy<StructType>(Node);
	}

	template<typename StructType>
	int32 GetLinkIDForNode(class UAnimGraphNode_Base* Node, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis)
	{
		const int32* pIndex = GetNodePropertyIndex<StructType>(Node, SearchMode);
		if (pIndex)
		{
			return (AnimNodeProperties.Num() - 1 - *pIndex); //@TODO: Crazysauce
		}
		return -1;
	}

	template<typename StructType>
	UStructProperty* GetPropertyForNode(class UUTGraphNode_Base* Node, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis)
	{
		const int32* pIndex = GetNodePropertyIndex<StructType>(Node, SearchMode);
		if (pIndex)
		{
			if (UStructProperty* UtilityTreeProperty = UTNodeProperties[UTNodeProperties.Num() - 1 - *pIndex])
			{
				if (UtilityTreeProperty->Struct->IsChildOf(StructType::StaticStruct()))
				{
					return UtilityTreeProperty;
				}
			}
		}

		return NULL;
	}

	template<typename StructType>
	StructType* GetPropertyInstance(UObject* Object, class UAnimGraphNode_Base* Node, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis)
	{
		UStructProperty* AnimationProperty = GetPropertyForNode<StructType>(Node);
		if (AnimationProperty)
		{
			return AnimationProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
		}

		return NULL;
	}

	template<typename StructType>
	StructType* GetPropertyInstance(UObject* Object, FGuid NodeGuid, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis)
	{
		const int32* pIndex = GetNodePropertyIndexFromGuid(NodeGuid, SearchMode);
		if (pIndex)
		{
			if (UStructProperty* AnimProperty = AnimNodeProperties[AnimNodeProperties.Num() - 1 - *pIndex])
			{
				if (AnimProperty->Struct->IsChildOf(StructType::StaticStruct()))
				{
					return AnimProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
				}
			}
		}

		return NULL;
	}

	template<typename StructType>
	StructType& GetPropertyInstanceChecked(UObject* Object, class UAnimGraphNode_Base* Node, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis)
	{
		const int32 Index = UtilityTreeBlueprintDebugData.NodePropertyToIndexMap.FindChecked(Node);
		UStructProperty* AnimationProperty = AnimNodeProperties[AnimNodeProperties.Num() - 1 - Index];
		check(AnimationProperty);
		check(AnimationProperty->Struct->IsChildOf(StructType::StaticStruct()));
		return AnimationProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
	}

	const int32* GetNodePropertyIndexFromGuid(FGuid Guid, EUTPropertySearchMode::Type SearchMode = EUTPropertySearchMode::OnlyThis);

#endif

	// UStruct interface
	virtual void Link(FArchive& Ar, bool bRelinkExistingProperties) override;
	// End of UStruct interface

	// UClass interface
	virtual void PurgeClass(bool bRecompilingOnLoad) override;
	virtual uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const override;
	// End of UClass interface
};

template<typename NodeType>
NodeType* GetNodeFromPropertyIndex(UObject* AnimInstanceObject, const IAnimClassInterface* UtilityTreeBlueprintClass, int32 PropertyIndex)
{
	if (PropertyIndex != INDEX_NONE)
	{
		UStructProperty* NodeProperty = UtilityTreeBlueprintClass->GetAnimNodeProperties()[UtilityTreeBlueprintClass->GetAnimNodeProperties().Num() - 1 - PropertyIndex]; //@TODO: Crazysauce
		check(NodeProperty->Struct == NodeType::StaticStruct());
		return NodeProperty->ContainerPtrToValuePtr<NodeType>(AnimInstanceObject);
	}

	return NULL;
}
