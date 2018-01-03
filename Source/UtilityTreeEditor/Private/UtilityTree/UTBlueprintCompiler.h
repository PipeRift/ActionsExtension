// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KismetCompiler.h"
#include "UTBlueprintGeneratedClass.h"
#include "Nodes/AINodeBase.h"
#include "UtilityTree/AIGraphNode_Base.h"

class UUtilityTreeGraphSchema;
class UK2Node_CallFunction;

//
// Forward declarations.
//
class UStructProperty;
class UBlueprintGeneratedClass;
struct FAILinkMappingRecord;

class UUtilityTreeBlueprint;


//////////////////////////////////////////////////////////////////////////
// FUTBlueprintCompiler

class UTILITYTREEEDITOR_API FUTBlueprintCompiler : public FKismetCompilerContext
{
protected:
	typedef FKismetCompilerContext Super;
public:
	FUTBlueprintCompiler(UUtilityTreeBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions, TArray<UObject*>* InObjLoaded);
	virtual ~FUTBlueprintCompiler();

	virtual void PostCompile() override;

protected:
	// Implementation of FKismetCompilerContext interface
	virtual void CreateClassVariablesFromBlueprint() override;
	virtual UEdGraphSchema_K2* CreateSchema() override;
	virtual void MergeUbergraphPagesIn(UEdGraph* Ubergraph) override;
	virtual void ProcessOneFunctionGraph(UEdGraph* SourceGraph, bool bInternalFunction = false) override;
	virtual void CreateFunctionList() override;
	virtual void SpawnNewClass(const FString& NewClassName) override;
	virtual void OnNewClassSet(UBlueprintGeneratedClass* ClassToUse) override;
	virtual void CopyTermDefaultsToDefaultObject(UObject* DefaultObject) override;
	virtual void EnsureProperGeneratedClass(UClass*& TargetClass) override;
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO) override;
	// End of FKismetCompilerContext interface

protected:
	typedef TArray<UEdGraphPin*> UEdGraphPinArray;

protected:
	/** Record of a single copy operation */
	struct FPropertyCopyRecord
	{
		FPropertyCopyRecord(UEdGraphPin* InDestPin, UProperty* InDestProperty, int32 InDestArrayIndex)
			: DestPin(InDestPin)
			, DestProperty(InDestProperty)
			, DestArrayIndex(InDestArrayIndex)
			, SourcePropertyName(NAME_None)
			, SourceSubStructPropertyName(NAME_None)
			, Operation(EPostCopyOperation::None)
		{}

		bool IsFastPath() const
		{
			return DestProperty != nullptr && SourcePropertyName != NAME_None;
		}

		void InvalidateFastPath()
		{
			SourcePropertyName = NAME_None;
			SourceSubStructPropertyName = NAME_None;
		}

		void ValidateFastPath(UClass* InCompiledClass);

		/** The destination pin we are copying to */
		UEdGraphPin* DestPin;

		/** The destination property we are copying to (on an animation node) */
		UProperty* DestProperty;

		/** The array index we use if the destination property is an array */
		int32 DestArrayIndex;

		/** The source property we are copying from (on an anim instance) */
		FName SourcePropertyName;

		/** The source sub-struct property we are copying from (if the source property is a struct property) */
		FName SourceSubStructPropertyName;

		/** Any operation we want to perform post-copy on the destination data */
		EPostCopyOperation Operation;
	};

	// Wireup record for a single anim node property (which might be an array)
	struct FUTNodeSinglePropertyHandler
	{
		/** Copy records */
		TArray<FPropertyCopyRecord> CopyRecords;

		// If the anim instance is the container target instead of the node.
		bool bInstanceIsTarget;

		FUTNodeSinglePropertyHandler()
			: bInstanceIsTarget(false)
		{
		}
	};

	// Record for a property that was exposed as a pin, but wasn't wired up (just a literal)
	struct FEffectiveConstantRecord
	{
	public:
		// The node variable that the handler is in
		class UStructProperty* NodeVariableProperty;

		// The property within the struct to set
		class UProperty* ConstantProperty;

		// The array index if ConstantProperty is an array property, or INDEX_NONE otherwise
		int32 ArrayIndex;

		// The pin to pull the DefaultValue/DefaultObject from
		UEdGraphPin* LiteralSourcePin;

		FEffectiveConstantRecord()
			: NodeVariableProperty(NULL)
			, ConstantProperty(NULL)
			, ArrayIndex(INDEX_NONE)
			, LiteralSourcePin(NULL)
		{
		}

		FEffectiveConstantRecord(UStructProperty* ContainingNodeProperty, UEdGraphPin* SourcePin, UProperty* SourcePinProperty, int32 SourceArrayIndex)
			: NodeVariableProperty(ContainingNodeProperty)
			, ConstantProperty(SourcePinProperty)
			, ArrayIndex(SourceArrayIndex)
			, LiteralSourcePin(SourcePin)
		{
		}

		bool Apply(UObject* Object);
	};

	struct FEvaluationHandlerRecord
	{
	public:

		// The node variable that the handler is in
		UStructProperty* NodeVariableProperty;

		// The specific evaluation handler inside the specified node
		UStructProperty* EvaluationHandlerProperty;

		// Whether or not our serviced properties are actually on the instance instead of the node
		bool bServicesInstanceProperties;

		// Set of properties serviced by this handler (Map from property name to the record for that property)
		TMap<FName, FUTNodeSinglePropertyHandler> ServicedProperties;

		// The resulting function name
		FName HandlerFunctionName;

	public:

		FEvaluationHandlerRecord()
			: NodeVariableProperty(nullptr)
			, EvaluationHandlerProperty(nullptr)
			, bServicesInstanceProperties(false)
			, HandlerFunctionName(NAME_None)
		{}

		bool IsFastPath() const
		{
			for(TMap<FName, FUTNodeSinglePropertyHandler>::TConstIterator It(ServicedProperties); It; ++It)
			{
				const FUTNodeSinglePropertyHandler& UTNodeSinglePropertyHandler = It.Value();
				for (const FPropertyCopyRecord& CopyRecord : UTNodeSinglePropertyHandler.CopyRecords)
				{
					if (!CopyRecord.IsFastPath())
					{
						return false;
					}
				}
			}

			return true;
		}

		bool IsValid() const
		{
			return NodeVariableProperty != nullptr && EvaluationHandlerProperty != nullptr;
		}

		void PatchFunctionNameAndCopyRecordsInto(UObject* TargetObject) const;

		void RegisterPin(UEdGraphPin* DestPin, UProperty* AssociatedProperty, int32 AssociatedPropertyArrayIndex);

		UStructProperty* GetHandlerNodeProperty() const { return NodeVariableProperty; }

		void BuildFastPathCopyRecords();

		void ValidateFastPath(UClass* InCompiledClass);

	private:

		bool CheckForVariableGet(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin);

		bool CheckForLogicalNot(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin);

		bool CheckForStructMemberAccess(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin);

		bool CheckForMemberOnlyAccess(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin);
	};

protected:
	UUTBlueprintGeneratedClass* NewUTBlueprintClass;
	UUtilityTreeBlueprint* UtilityTreeBlueprint;

	UUtilityTreeGraphSchema* UTSchema;

	// Map of allocated v3 nodes that are members of the class
	TMap<class UAIGraphNode_Base*, UProperty*> AllocatedUTNodes;
	TMap<UProperty*, class UAIGraphNode_Base*> AllocatedNodePropertiesToNodes;
	TMap<int32, UProperty*> AllocatedPropertiesByIndex;

	// Map of true source objects (user edited ones) to the cloned ones that are actually compiled
	TMap<class UAIGraphNode_Base*, UAIGraphNode_Base*> SourceNodeToProcessedNodeMap;

	// Index of the nodes (must match up with the runtime discovery process of nodes, which runs through the property chain)
	int32 AllocateNodeIndexCounter;
	TMap<class UAIGraphNode_Base*, int32> AllocatedUTNodeIndices;

	// Map from pose link LinkID address
	//@TODO: Bad structure for a list of these
	TArray<FAILinkMappingRecord> ValidPoseLinkList;

	// List of successfully created evaluation handlers
	TArray<FEvaluationHandlerRecord> ValidEvaluationHandlerList;

	// List of utility tree node literals (values exposed as pins but never wired up) that need to be pushed into the CDO
	TArray<FEffectiveConstantRecord> ValidUTNodePinConstants;

	// List of getter node's we've found so the auto-wire can be deferred till after state machine compilation
	TArray<class UK2Node_UTGetter*> FoundGetterNodes;

	// Set of used handler function names
	TSet<FName> HandlerFunctionNames;

	// True if any parent class is also generated from an animation blueprint
	bool bIsDerivedUtilityTreeBlueprint;

private:

	UK2Node_CallFunction* SpawnCallUtilityTreeFunction(UEdGraphNode* SourceNode, FName FunctionName);

	// Creates an evaluation handler for an FExposedValue property in an utility tree node
	void CreateEvaluationHandlerStruct(UAIGraphNode_Base* VisualUTNode, FEvaluationHandlerRecord& Record);
	void CreateEvaluationHandlerInstance(UAIGraphNode_Base* VisualUTNode, FEvaluationHandlerRecord& Record);

	// Prunes any nodes that aren't reachable via a pose link
	void PruneIsolatedUtilityTreeNodes(const TArray<UAIGraphNode_Base*>& RootSet, TArray<UAIGraphNode_Base*>& GraphNodes);

	// Compiles one animation node
	void ProcessUtilityTreeNode(UAIGraphNode_Base* VisualUTNode);

	// Compiles an entire animation graph
	void ProcessAllUtilityTreeNodes();

	
	void ProcessUTNodesGivenRoot(TArray<UAIGraphNode_Base*>& UTNodeList, const TArray<UAIGraphNode_Base*>& RootSet);

	// Gets all utility tree graph nodes that are piped into the provided node (traverses input pins)
	void GetLinkedUTNodes(UAIGraphNode_Base* InGraphNode, TArray<UAIGraphNode_Base*>& LinkedUTNodes);
	void GetLinkedUTNodes_TraversePin(UEdGraphPin* InPin, TArray<UAIGraphNode_Base*>& LinkedUTNodes);
	void GetLinkedUTNodes_ProcessUTNode(UAIGraphNode_Base* UTNode, TArray<UAIGraphNode_Base*>& LinkedUTNodes);

	// Automatically fill in parameters for the specified Getter node
	//void AutoWireUTGetter(class UK2Node_UTGetter* Getter, UUTStateTransitionNode* InTransitionNode);

	// Dumps compiler diagnostic information
	void DumpUTDebugData();

	// Returns the allocation index of the specified node, processing it if it was pending
	int32 GetAllocationIndexOfNode(UAIGraphNode_Base* VisualUTNode);
};

