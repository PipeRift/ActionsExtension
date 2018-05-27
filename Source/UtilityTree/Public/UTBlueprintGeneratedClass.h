// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Engine/PoseWatch.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Animation/AnimClassInterface.h"

#include "UTBlueprintGeneratedClass.generated.h"

class UAIGraphNode_Base;
class UUtilityTree;
class UEdGraph;


struct FUTNodePoseWatch
{
    TSharedPtr<FCompactHeapPose>    PoseInfo;
    FColor                            PoseDrawColour;
    int32                            NodeID;
};

// This structure represents debugging information for a frame snapshot
USTRUCT()
struct FUTFrameSnapshot
{
    GENERATED_USTRUCT_BODY()

    FUTFrameSnapshot()
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
    UTILITYTREE_API void CopyToInstance(UUtilityTree* Instance);
#endif
};

// This structure represents animation-related debugging information for an entire UtilityTreeBlueprint
// (general debug information for the event graph, etc... is still contained in a FBlueprintDebugData structure)
USTRUCT()
struct UTILITYTREE_API FUTBlueprintDebugData
{
    GENERATED_USTRUCT_BODY()

    FUTBlueprintDebugData()
#if WITH_EDITORONLY_DATA
        : SnapshotBuffer(NULL)
        , SnapshotIndex(INDEX_NONE)
#endif
    {
    }

#if WITH_EDITORONLY_DATA
public:

    // Map from state graphs to their node
    TMap<TWeakObjectPtr<UEdGraph>, TWeakObjectPtr<class UAnimStateNode> > StateGraphToNodeMap;

    // Map from animation node to their property index
    TMap<TWeakObjectPtr<class UAIGraphNode_Base>, int32> NodePropertyToIndexMap;

    // Map from animation node GUID to property index
    TMap<FGuid, int32> NodeGuidToIndexMap;

    // History of snapshots of animation data
    TSimpleRingBuffer<FUTFrameSnapshot>* SnapshotBuffer;

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
    TArray<FUTNodePoseWatch> UTNodePoseWatch;

    // Index of snapshot
    int32 SnapshotIndex;
public:

    ~FUTBlueprintDebugData()
    {
        if (SnapshotBuffer != NULL)
        {
            delete SnapshotBuffer;
        }
        SnapshotBuffer = NULL;
    }



    bool IsReplayingSnapshot() const { return SnapshotIndex != INDEX_NONE; }
    void TakeSnapshot(UUtilityTree* Instance);
    float GetSnapshotLengthInSeconds();
    int32 GetSnapshotLengthInFrames();
    void SetSnapshotIndexByTime(UUtilityTree* Instance, double TargetTime);
    void SetSnapshotIndex(UUtilityTree* Instance, int32 NewIndex);
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
namespace EAIPropertySearchMode
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
    int32 RootUTNodeIndex;

    // Indices for each of the saved pose nodes that require updating, in the order they need to get updates.
    UPROPERTY()
    TArray<int32> OrderedSavedPoseIndices;

    // The array of anim nodes; this is transient generated data (created during Link)
    UStructProperty* RootUTNodeProperty;
    TArray<UStructProperty*> UTNodeProperties;

public:

    virtual int32 GetRootUTNodeIndex() const { return RootUTNodeIndex; }

    virtual UStructProperty* GetRootUTNodeProperty() const { return RootUTNodeProperty; }

    virtual const TArray<UStructProperty*>& GetUTNodeProperties() const { return UTNodeProperties; }

public:
#if WITH_EDITORONLY_DATA
    FUTBlueprintDebugData UTBlueprintDebugData;

    FUTBlueprintDebugData& GetUTBlueprintDebugData()
    {
        return UTBlueprintDebugData;
    }

    template<typename StructType>
    const int32* GetNodePropertyIndexFromHierarchy(class UAIGraphNode_Base* Node)
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
    const int32* GetNodePropertyIndex(class UAnimGraphNode_Base* Node, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis)
    {
        return (SearchMode == EAIPropertySearchMode::OnlyThis) ? UTBlueprintDebugData.NodePropertyToIndexMap.Find(Node) : GetNodePropertyIndexFromHierarchy<StructType>(Node);
    }

    template<typename StructType>
    int32 GetLinkIDForNode(class UAnimGraphNode_Base* Node, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis)
    {
        const int32* pIndex = GetNodePropertyIndex<StructType>(Node, SearchMode);
        if (pIndex)
        {
            return (AnimNodeProperties.Num() - 1 - *pIndex); //@TODO: Crazysauce
        }
        return -1;
    }

    template<typename StructType>
    UStructProperty* GetPropertyForNode(class UAIGraphNode_Base* Node, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis)
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
    StructType* GetPropertyInstance(UObject* Object, class UAIGraphNode_Base* Node, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis)
    {
        UStructProperty* AIProperty = GetPropertyForNode<StructType>(Node);
        if (AIProperty)
        {
            return AIProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
        }

        return NULL;
    }

    template<typename StructType>
    StructType* GetPropertyInstance(UObject* Object, FGuid NodeGuid, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis)
    {
        const int32* pIndex = GetNodePropertyIndexFromGuid(NodeGuid, SearchMode);
        if (pIndex)
        {
            if (UStructProperty* UTProperty = UTNodeProperties[UTNodeProperties.Num() - 1 - *pIndex])
            {
                if (UTProperty->Struct->IsChildOf(StructType::StaticStruct()))
                {
                    return UTProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
                }
            }
        }

        return NULL;
    }

    template<typename StructType>
    StructType& GetPropertyInstanceChecked(UObject* Object, class UAnimGraphNode_Base* Node, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis)
    {
        const int32 Index = UtilityTreeBlueprintDebugData.NodePropertyToIndexMap.FindChecked(Node);
        UStructProperty* UTProperty = UTNodeProperties[UTNodeProperties.Num() - 1 - Index];
        check(UTProperty);
        check(UTProperty->Struct->IsChildOf(StructType::StaticStruct()));
        return UTProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
    }

    const int32* GetNodePropertyIndexFromGuid(FGuid Guid, EAIPropertySearchMode::Type SearchMode = EAIPropertySearchMode::OnlyThis);

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
NodeType* GetNodeFromPropertyIndex(UObject* UtilityTreeObject, const UUTBlueprintGeneratedClass* UtilityTreeBlueprintClass, int32 PropertyIndex)
{
    if (PropertyIndex != INDEX_NONE)
    {
        UStructProperty* NodeProperty = UtilityTreeBlueprintClass->GetUTNodeProperties()[UtilityTreeBlueprintClass->GetUTNodeProperties().Num() - 1 - PropertyIndex];
        check(NodeProperty->Struct == NodeType::StaticStruct());
        return NodeProperty->ContainerPtrToValuePtr<NodeType>(UtilityTreeObject);
    }

    return NULL;
}
