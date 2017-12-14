// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UtilityTree/UTBlueprintCompiler.h"
#include "UObject/UObjectHash.h"
#include "Animation/AnimInstance.h"
#include "EdGraphUtilities.h"
#include "K2Node_CallFunction.h"
#include "K2Node_StructMemberGet.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Knot.h"
#include "K2Node_StructMemberSet.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetReinstanceUtilities.h"

#include "UtilityTree/UtilityTreeBlueprint.h"
#include "UtilityTree/UtilityTreeGraphSchema.h"
#include "UtilityTree/UTGraphNode_Root.h"

//#include "UtilityTreeBlueprintPostCompileValidation.h" 

#define LOCTEXT_NAMESPACE "UTBlueprintCompiler"

//////////////////////////////////////////////////////////////////////////
// FUTBlueprintCompiler::FEffectiveConstantRecord

bool FUTBlueprintCompiler::FEffectiveConstantRecord::Apply(UObject* Object)
{
	uint8* StructPtr = nullptr;
	uint8* PropertyPtr = nullptr;
	
	if(NodeVariableProperty->Struct == FUTNode_SubInstance::StaticStruct())
	{
		PropertyPtr = ConstantProperty->ContainerPtrToValuePtr<uint8>(Object);
	}
	else
	{
		StructPtr = NodeVariableProperty->ContainerPtrToValuePtr<uint8>(Object);
		PropertyPtr = ConstantProperty->ContainerPtrToValuePtr<uint8>(StructPtr);
	}

	if (ArrayIndex != INDEX_NONE)
	{
		UArrayProperty* ArrayProperty = CastChecked<UArrayProperty>(ConstantProperty);

		// Peer inside the array
		FScriptArrayHelper ArrayHelper(ArrayProperty, PropertyPtr);

		if (ArrayHelper.IsValidIndex(ArrayIndex))
		{
			FBlueprintEditorUtils::PropertyValueFromString_Direct(ArrayProperty->Inner, LiteralSourcePin->GetDefaultAsString(), ArrayHelper.GetRawPtr(ArrayIndex));
		}
		else
		{
			return false;
		}
	}
	else
	{
		FBlueprintEditorUtils::PropertyValueFromString_Direct(ConstantProperty, LiteralSourcePin->GetDefaultAsString(), PropertyPtr);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// FUTBlueprintCompiler

FUTBlueprintCompiler::FUTBlueprintCompiler(UUtilityTreeBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions, TArray<UObject*>* InObjLoaded)
	: FKismetCompilerContext(SourceSketch, InMessageLog, InCompileOptions, InObjLoaded)
	, UtilityTreeBlueprint(SourceSketch)
	, bIsDerivedUtilityTreeBlueprint(false)
{
	// Make sure the skeleton has finished preloading
	if (UtilityTreeBlueprint->TargetSkeleton != nullptr)
	{
		if (FLinkerLoad* Linker = UtilityTreeBlueprint->TargetSkeleton->GetLinker())
		{
			Linker->Preload(UtilityTreeBlueprint->TargetSkeleton);
		}
	}

	// Determine if there is an utility tree blueprint in the ancestry of this class
	bIsDerivedUtilityTreeBlueprint = UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(UtilityTreeBlueprint) != NULL;
}

FUTBlueprintCompiler::~FUTBlueprintCompiler()
{
}

void FUTBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	FKismetCompilerContext::CreateClassVariablesFromBlueprint();

	if(bGenerateSubInstanceVariables)
	{	
		for (UEdGraph* It : Blueprint->UbergraphPages)
		{
			TArray<UUTGraphNode_SubInstance*> SubInstanceNodes;
			It->GetNodesOfClass(SubInstanceNodes);
			for( UUTGraphNode_SubInstance* SubInstance : SubInstanceNodes )
			{
				ProcessSubInstance(SubInstance, false);
			}
		}
		
		if(!bIsDerivedUtilityTreeBlueprint)
		{
			for (UEdGraph* It : Blueprint->FunctionGraphs)
			{
				TArray<UUTGraphNode_SubInstance*> SubInstanceNodes;
				It->GetNodesOfClass(SubInstanceNodes);
				for( UUTGraphNode_SubInstance* SubInstance : SubInstanceNodes )
				{
					ProcessSubInstance(SubInstance, false);
				}
			}
		}
	}
}


UEdGraphSchema_K2* FUTBlueprintCompiler::CreateSchema()
{
	AnimSchema = NewObject<UAnimationGraphSchema>();
	return AnimSchema;
}

UK2Node_CallFunction* FUTBlueprintCompiler::SpawnCallAnimInstanceFunction(UEdGraphNode* SourceNode, FName FunctionName)
{
	//@TODO: SKELETON: This is a call on a parent function (UAnimInstance::StaticClass() specifically), should we treat it as self or not?
	UK2Node_CallFunction* FunctionCall = SpawnIntermediateNode<UK2Node_CallFunction>(SourceNode);
	FunctionCall->FunctionReference.SetSelfMember(FunctionName);
	FunctionCall->AllocateDefaultPins();

	return FunctionCall;
}

void FUTBlueprintCompiler::CreateEvaluationHandlerStruct(UUTGraphNode_Base* VisualAnimNode, FEvaluationHandlerRecord& Record)
{
	// Shouldn't create a handler if there is nothing to work with
	check(Record.ServicedProperties.Num() > 0);
	check(Record.NodeVariableProperty != NULL);

	// Use the node GUID for a stable name across compiles
	FString FunctionName = FString::Printf(TEXT("%s_%s_%s_%s"), *Record.EvaluationHandlerProperty->GetName(), *VisualAnimNode->GetOuter()->GetName(), *VisualAnimNode->GetClass()->GetName(), *VisualAnimNode->NodeGuid.ToString());
	Record.HandlerFunctionName = FName(*FunctionName);

	// check function name isnt already used (data exists that can contain duplicate GUIDs) and apply a numeric extension until it is unique
	int32 ExtensionIndex = 0;
	FName* ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
	while(ExistingName != nullptr)
	{
		FunctionName = FString::Printf(TEXT("%s_%s_%s_%s_%d"), *Record.EvaluationHandlerProperty->GetName(), *VisualAnimNode->GetOuter()->GetName(), *VisualAnimNode->GetClass()->GetName(), *VisualAnimNode->NodeGuid.ToString(), ExtensionIndex);
		Record.HandlerFunctionName = FName(*FunctionName);
		ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
		ExtensionIndex++;
	}

	HandlerFunctionNames.Add(Record.HandlerFunctionName);
	
	// Add a custom event in the graph
	UK2Node_CustomEvent* EntryNode = SpawnIntermediateEventNode<UK2Node_CustomEvent>(VisualAnimNode, nullptr, ConsolidatedEventGraph);
	EntryNode->bInternalEvent = true;
	EntryNode->CustomFunctionName = Record.HandlerFunctionName;
	EntryNode->AllocateDefaultPins();

	// The ExecChain is the current exec output pin in the linear chain
	UEdGraphPin* ExecChain = Schema->FindExecutionPin(*EntryNode, EGPD_Output);

	// Create a struct member write node to store the parameters into the animation node
	UK2Node_StructMemberSet* AssignmentNode = SpawnIntermediateNode<UK2Node_StructMemberSet>(VisualAnimNode, ConsolidatedEventGraph);
	AssignmentNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
	AssignmentNode->StructType = Record.NodeVariableProperty->Struct;
	AssignmentNode->AllocateDefaultPins();

	// Wire up the variable node execution wires
	UEdGraphPin* ExecVariablesIn = Schema->FindExecutionPin(*AssignmentNode, EGPD_Input);
	ExecChain->MakeLinkTo(ExecVariablesIn);
	ExecChain = Schema->FindExecutionPin(*AssignmentNode, EGPD_Output);

	// Run thru each property
	TSet<FName> PropertiesBeingSet;

	for (auto TargetPinIt = AssignmentNode->Pins.CreateIterator(); TargetPinIt; ++TargetPinIt)
	{
		UEdGraphPin* TargetPin = *TargetPinIt;
		FString PropertyNameStr = TargetPin->PinName;
		FName PropertyName(*PropertyNameStr);

		// Does it get serviced by this handler?
		if (FUTNodeSinglePropertyHandler* SourceInfo = Record.ServicedProperties.Find(PropertyName))
		{
			if (TargetPin->PinType.IsArray())
			{
				// Grab the array that we need to set members for
				UK2Node_StructMemberGet* FetchArrayNode = SpawnIntermediateNode<UK2Node_StructMemberGet>(VisualAnimNode, ConsolidatedEventGraph);
				FetchArrayNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
				FetchArrayNode->StructType = Record.NodeVariableProperty->Struct;
				FetchArrayNode->AllocatePinsForSingleMemberGet(PropertyName);

				UEdGraphPin* ArrayVariableNode = FetchArrayNode->FindPin(PropertyNameStr);

				if (SourceInfo->CopyRecords.Num() > 0)
				{
					// Set each element in the array
					for (FPropertyCopyRecord& CopyRecord : SourceInfo->CopyRecords)
					{
						int32 ArrayIndex = CopyRecord.DestArrayIndex;
						UEdGraphPin* DestPin = CopyRecord.DestPin;

						// Create an array element set node
						UK2Node_CallArrayFunction* ArrayNode = SpawnIntermediateNode<UK2Node_CallArrayFunction>(VisualAnimNode, ConsolidatedEventGraph);
						ArrayNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Set), UKismetArrayLibrary::StaticClass());
						ArrayNode->AllocateDefaultPins();

						// Connect the execution chain
						ExecChain->MakeLinkTo(ArrayNode->GetExecPin());
						ExecChain = ArrayNode->GetThenPin();

						// Connect the input array
						UEdGraphPin* TargetArrayPin = ArrayNode->FindPinChecked(TEXT("TargetArray"));
						TargetArrayPin->MakeLinkTo(ArrayVariableNode);
						ArrayNode->PinConnectionListChanged(TargetArrayPin);

						// Set the array index
						UEdGraphPin* TargetIndexPin = ArrayNode->FindPinChecked(TEXT("Index"));
						TargetIndexPin->DefaultValue = FString::FromInt(ArrayIndex);

						// Wire up the data input
						UEdGraphPin* TargetItemPin = ArrayNode->FindPinChecked(TEXT("Item"));
						TargetItemPin->CopyPersistentDataFromOldPin(*DestPin);
						MessageLog.NotifyIntermediatePinCreation(TargetItemPin, DestPin);
						DestPin->BreakAllPinLinks();
					}
				}
			}
			else
			{
				check(!TargetPin->PinType.IsContainer())
				// Single property
				if (SourceInfo->CopyRecords.Num() > 0 && SourceInfo->CopyRecords[0].DestPin != nullptr)
				{
					UEdGraphPin* DestPin = SourceInfo->CopyRecords[0].DestPin;

					PropertiesBeingSet.Add(FName(*DestPin->PinName));
					TargetPin->CopyPersistentDataFromOldPin(*DestPin);
					MessageLog.NotifyIntermediatePinCreation(TargetPin, DestPin);
					DestPin->BreakAllPinLinks();
				}
			}
		}
	}

	// Remove any unused pins from the assignment node to avoid smashing constant values
	for (int32 PinIndex = 0; PinIndex < AssignmentNode->ShowPinForProperties.Num(); ++PinIndex)
	{
		FOptionalPinFromProperty& TestProperty = AssignmentNode->ShowPinForProperties[PinIndex];
		TestProperty.bShowPin = PropertiesBeingSet.Contains(TestProperty.PropertyName);
	}
	AssignmentNode->ReconstructNode();
}

void FUTBlueprintCompiler::CreateEvaluationHandlerInstance(UUTGraphNode_Base* VisualAnimNode, FEvaluationHandlerRecord& Record)
{
	// Shouldn't create a handler if there is nothing to work with
	check(Record.ServicedProperties.Num() > 0);
	check(Record.NodeVariableProperty != nullptr);
	check(Record.bServicesInstanceProperties);

	// Use the node GUID for a stable name across compiles
	FString FunctionName = FString::Printf(TEXT("%s_%s_%s_%s"), *Record.EvaluationHandlerProperty->GetName(), *VisualAnimNode->GetOuter()->GetName(), *VisualAnimNode->GetClass()->GetName(), *VisualAnimNode->NodeGuid.ToString());
	Record.HandlerFunctionName = FName(*FunctionName);

	// check function name isnt already used (data exists that can contain duplicate GUIDs) and apply a numeric extension until it is unique
	int32 ExtensionIndex = 0;
	FName* ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
	while(ExistingName != nullptr)
	{
		FunctionName = FString::Printf(TEXT("%s_%s_%s_%s_%d"), *Record.EvaluationHandlerProperty->GetName(), *VisualAnimNode->GetOuter()->GetName(), *VisualAnimNode->GetClass()->GetName(), *VisualAnimNode->NodeGuid.ToString(), ExtensionIndex);
		Record.HandlerFunctionName = FName(*FunctionName);
		ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
		ExtensionIndex++;
	}

	HandlerFunctionNames.Add(Record.HandlerFunctionName);

	// Add a custom event in the graph
	UK2Node_CustomEvent* EntryNode = SpawnIntermediateNode<UK2Node_CustomEvent>(VisualAnimNode, ConsolidatedEventGraph);
	EntryNode->bInternalEvent = true;
	EntryNode->CustomFunctionName = Record.HandlerFunctionName;
	EntryNode->AllocateDefaultPins();

	// The ExecChain is the current exec output pin in the linear chain
	UEdGraphPin* ExecChain = Schema->FindExecutionPin(*EntryNode, EGPD_Output);

	// Need to create a variable set call for each serviced property in the handler
	for(TPair<FName, FUTNodeSinglePropertyHandler>& PropHandlerPair : Record.ServicedProperties)
	{
		FUTNodeSinglePropertyHandler& PropHandler = PropHandlerPair.Value;
		FName PropertyName = PropHandlerPair.Key;

		// Should be true, we only want to deal with instance targets in here
		check(PropHandler.bInstanceIsTarget);

		for(FPropertyCopyRecord& CopyRecord : PropHandler.CopyRecords)
		{
			// New set node for the property
			UK2Node_VariableSet* VarAssignNode = SpawnIntermediateNode<UK2Node_VariableSet>(VisualAnimNode, ConsolidatedEventGraph);
			VarAssignNode->VariableReference.SetSelfMember(CopyRecord.DestProperty->GetFName());
			VarAssignNode->AllocateDefaultPins();

			// Wire up the exec line, and update the end of the chain
			UEdGraphPin* ExecVariablesIn = Schema->FindExecutionPin(*VarAssignNode, EGPD_Input);
			ExecChain->MakeLinkTo(ExecVariablesIn);
			ExecChain = Schema->FindExecutionPin(*VarAssignNode, EGPD_Output);

			// Find the property pin on the set node and configure
			for(UEdGraphPin* TargetPin : VarAssignNode->Pins)
			{
				if(TargetPin->PinType.IsContainer())
				{
					// Currently unsupported
					continue;
				}

				FString PropertyNameStr = TargetPin->PinName;
				FName PinPropertyName(*PropertyNameStr);

				if(PinPropertyName == PropertyName)
				{
					// This is us, wire up the variable
					UEdGraphPin* DestPin = CopyRecord.DestPin;

					// Copy the data (link up to the source nodes)
					TargetPin->CopyPersistentDataFromOldPin(*DestPin);
					MessageLog.NotifyIntermediatePinCreation(TargetPin, DestPin);

					// Old pin needs to not be connected now - break all its links
					DestPin->BreakAllPinLinks();

					break;
				}
			}

			//VarAssignNode->ReconstructNode();
		}
	}
}

void FUTBlueprintCompiler::ProcessUtilityTreeNode(UUTGraphNode_Base* VisualAnimNode)
{
	// Early out if this node has already been processed
	if (AllocatedAnimNodes.Contains(VisualAnimNode))
	{
		return;
	}

	// Make sure the visual node has a runtime node template
	const UScriptStruct* NodeType = VisualAnimNode->GetFNodeType();
	if (NodeType == NULL)
	{
		MessageLog.Error(TEXT("@@ has no animation node member"), VisualAnimNode);
		return;
	}

	// Give the visual node a chance to do validation
	{
		const int32 PreValidationErrorCount = MessageLog.NumErrors;
		VisualAnimNode->ValidateAnimNodeDuringCompilation(UtilityTreeBlueprint->TargetSkeleton, MessageLog);
		VisualAnimNode->BakeDataDuringCompilation(MessageLog);
		if (MessageLog.NumErrors != PreValidationErrorCount)
		{
			return;
		}
	}

	// Create a property for the node
	const FString NodeVariableName = ClassScopeNetNameMap.MakeValidName(VisualAnimNode);

	const UAnimationGraphSchema* AnimGraphDefaultSchema = GetDefault<UAnimationGraphSchema>();

	FEdGraphPinType NodeVariableType;
	NodeVariableType.PinCategory = AnimGraphDefaultSchema->PC_Struct;
	NodeVariableType.PinSubCategoryObject = NodeType;

	UStructProperty* NewProperty = Cast<UStructProperty>(CreateVariable(FName(*NodeVariableName), NodeVariableType));

	if (NewProperty == NULL)
	{
		MessageLog.Error(TEXT("Failed to create node property for @@"), VisualAnimNode);
	}

	// Register this node with the compile-time data structures
	const int32 AllocatedIndex = AllocateNodeIndexCounter++;
	AllocatedAnimNodes.Add(VisualAnimNode, NewProperty);
	AllocatedNodePropertiesToNodes.Add(NewProperty, VisualAnimNode);
	AllocatedAnimNodeIndices.Add(VisualAnimNode, AllocatedIndex);
	AllocatedPropertiesByIndex.Add(AllocatedIndex, NewProperty);

	UUTGraphNode_Base* TrueSourceObject = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(VisualAnimNode);
	SourceNodeToProcessedNodeMap.Add(TrueSourceObject, VisualAnimNode);

	// Register the slightly more permanent debug information
	NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().NodePropertyToIndexMap.Add(TrueSourceObject, AllocatedIndex);
	NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().NodeGuidToIndexMap.Add(TrueSourceObject->NodeGuid, AllocatedIndex);
	NewUtilityTreeBlueprintClass->GetDebugData().RegisterClassPropertyAssociation(TrueSourceObject, NewProperty);

	// Node-specific compilation that requires compiler state info
	/*if (UUTGraphNode_StateMachineBase* StateMachineInstance = Cast<UUTGraphNode_StateMachineBase>(VisualAnimNode))
	{
		// Compile the state machine
		ProcessStateMachine(StateMachineInstance);
	}*/

	// Record pose pins for later patchup and gather pins that have an associated evaluation handler
	TMap<FName, FEvaluationHandlerRecord> StructEvalHandlers;

	for (auto SourcePinIt = VisualAnimNode->Pins.CreateIterator(); SourcePinIt; ++SourcePinIt)
	{
		UEdGraphPin* SourcePin = *SourcePinIt;
		bool bConsumed = false;

		// Register pose links for future use
		if ((SourcePin->Direction == EGPD_Input) && (AnimGraphDefaultSchema->IsPosePin(SourcePin->PinType)))
		{
			// Input pose pin, going to need to be linked up
			FPoseLinkMappingRecord LinkRecord = VisualAnimNode->GetLinkIDLocation(NodeType, SourcePin);
			if (LinkRecord.IsValid())
			{
				ValidPoseLinkList.Add(LinkRecord);
				bConsumed = true;
			}
		}
		else
		{
			// The property source for our data, either the struct property for an anim node, or the
			// owning anim instance if using a sub instance node.
			UProperty* SourcePinProperty = nullptr;
			int32 SourceArrayIndex = INDEX_NONE;

			VisualAnimNode->GetPinAssociatedProperty(NodeType, SourcePin, /*out*/ SourcePinProperty, /*out*/ SourceArrayIndex);

			
			if (SourcePinProperty != NULL)
			{
				if (SourcePin->LinkedTo.Num() == 0)
				{
					// Literal that can be pushed into the CDO instead of re-evaluated every frame
					new (ValidUTNodePinConstants) FEffectiveConstantRecord(NewProperty, SourcePin, SourcePinProperty, SourceArrayIndex);
					bConsumed = true;
				}
				else
				{
					// Dynamic value that needs to be wired up and evaluated each frame
					FString EvaluationHandlerStr = SourcePinProperty->GetMetaData(AnimGraphDefaultSchema->NAME_OnEvaluate);
					FName EvaluationHandlerName(*EvaluationHandlerStr);
					if (EvaluationHandlerName == NAME_None)
					{
						EvaluationHandlerName = AnimGraphDefaultSchema->DefaultEvaluationHandlerName;
					}

					FEvaluationHandlerRecord& EvalHandler = StructEvalHandlers.FindOrAdd(EvaluationHandlerName);

					EvalHandler.RegisterPin(SourcePin, SourcePinProperty, SourceArrayIndex);

					bConsumed = true;
				}

				UEdGraphPin* TrueSourcePin = MessageLog.FindSourcePin(SourcePin);
				if (TrueSourcePin)
				{
					NewUtilityTreeBlueprintClass->GetDebugData().RegisterClassPropertyAssociation(TrueSourcePin, SourcePinProperty);
				}
			}
		}

		if (!bConsumed && (SourcePin->Direction == EGPD_Input))
		{
			//@TODO: ANIMREFACTOR: It's probably OK to have certain pins ignored eventually, but this is very helpful during development
			MessageLog.Note(TEXT("@@ was visible but ignored"), SourcePin);
		}
	}

	// Match the associated property to each evaluation handler
	for (TFieldIterator<UProperty> NodePropIt(NodeType); NodePropIt; ++NodePropIt)
	{
		if (UStructProperty* StructProp = Cast<UStructProperty>(*NodePropIt))
		{
			if (StructProp->Struct == FExposedValueHandler::StaticStruct())
			{
				// Register this property to the list of pins that need to be updated
				// (it's OK if there isn't an entry for this handler, it means that the values are static and don't need to be calculated every frame)
				FName EvaluationHandlerName(StructProp->GetFName());

				FEvaluationHandlerRecord* pRecord = StructEvalHandlers.Find(EvaluationHandlerName);

				if (pRecord)
				{
					pRecord->NodeVariableProperty = NewProperty;
					pRecord->EvaluationHandlerProperty = StructProp;
				}
				
			}
		}
	}

	// Generate a new event to update the value of these properties
	for (auto HandlerIt = StructEvalHandlers.CreateIterator(); HandlerIt; ++HandlerIt)
	{
		FName EvaluationHandlerName = HandlerIt.Key();
		FEvaluationHandlerRecord& Record = HandlerIt.Value();

		if (Record.IsValid())
		{
			// build fast path copy records here
			// we need to do this at this point as they rely on traversing the original wire path
			// to determine source data. After we call CreateEvaluationHandlerStruct (etc) the original 
			// graph is modified to hook up to the evaluation handler custom functions & pins are no longer
			// available
			Record.BuildFastPathCopyRecords();

			if(Record.bServicesInstanceProperties)
			{
				CreateEvaluationHandlerInstance(VisualAnimNode, Record);
			}
			else
			{
				CreateEvaluationHandlerStruct(VisualAnimNode, Record);
			}

			ValidEvaluationHandlerList.Add(Record);
		}
		else
		{
			MessageLog.Error(*FString::Printf(TEXT("A property on @@ references a non-existent %s property named %s"), *(AnimGraphDefaultSchema->NAME_OnEvaluate.ToString()), *(EvaluationHandlerName.ToString())), VisualAnimNode);
		}
	}
}

int32 FUTBlueprintCompiler::GetAllocationIndexOfNode(UUTGraphNode_Base* VisualUTNode)
{
	ProcessUtilityTreeNode(VisualUTNode);
	int32* pResult = AllocatedUTNodeIndices.Find(VisualUTNode);
	return (pResult != NULL) ? *pResult : INDEX_NONE;
}

void FUTBlueprintCompiler::PruneIsolatedUtilityTreeNodes(const TArray<UUTGraphNode_Base*>& RootSet, TArray<UUTGraphNode_Base*>& GraphNodes)
{
	struct FNodeVisitorDownPoseWires
	{
		TSet<UEdGraphNode*> VisitedNodes;
		const UUtilityTreeGraphSchema* Schema;

		FNodeVisitorDownPoseWires()
		{
			Schema = GetDefault<UUtilityTreeGraphSchema>();
		}

		void TraverseNodes(UEdGraphNode* Node)
		{
			VisitedNodes.Add(Node);

			// Follow every exec output pin
			for (int32 i = 0; i < Node->Pins.Num(); ++i)
			{
				UEdGraphPin* MyPin = Node->Pins[i];

				if ((MyPin->Direction == EGPD_Input) && (Schema->IsPosePin(MyPin->PinType)))
				{
					for (int32 j = 0; j < MyPin->LinkedTo.Num(); ++j)
					{
						UEdGraphPin* OtherPin = MyPin->LinkedTo[j];
						UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
						if (!VisitedNodes.Contains(OtherNode))
						{
							TraverseNodes(OtherNode);
						}
					}
				}
			}
		}
	};

	// Prune the nodes that aren't reachable via an animation pose link
	FNodeVisitorDownPoseWires Visitor;

	for (auto RootIt = RootSet.CreateConstIterator(); RootIt; ++RootIt)
	{
		UUTGraphNode_Base* RootNode = *RootIt;
		Visitor.TraverseNodes(RootNode);
	}

	for (int32 NodeIndex = 0; NodeIndex < GraphNodes.Num(); ++NodeIndex)
	{
		UUTGraphNode_Base* Node = GraphNodes[NodeIndex];
		if (!Visitor.VisitedNodes.Contains(Node) && !IsNodePure(Node))
		{
			Node->BreakAllNodeLinks();
			GraphNodes.RemoveAtSwap(NodeIndex);
			--NodeIndex;
		}
	}
}

void FUTBlueprintCompiler::ProcessUTNodesGivenRoot(TArray<UUTGraphNode_Base*>& UTNodeList, const TArray<UUTGraphNode_Base*>& RootSet)
{
	// Now prune based on the root set
	if (MessageLog.NumErrors == 0)
	{
		PruneIsolatedAnimationNodes(RootSet, UTNodeList);
	}

	// Process the remaining nodes
	for (auto SourceNodeIt = UTNodeList.CreateIterator(); SourceNodeIt; ++SourceNodeIt)
	{
		UUTGraphNode_Base* VisualUTNode = *SourceNodeIt;
		ProcessUtilityTreeNode(VisualUTNode);
	}
}

void FUTBlueprintCompiler::GetLinkedUTNodes(UUTGraphNode_Base* InGraphNode, TArray<UUTGraphNode_Base*> &LinkedUTNodes)
{
	for(UEdGraphPin* Pin : InGraphNode->Pins)
	{
		if(Pin->Direction == EEdGraphPinDirection::EGPD_Input &&
		   Pin->PinType.PinCategory == TEXT("struct"))
		{
			if(UScriptStruct* Struct = Cast<UScriptStruct>(Pin->PinType.PinSubCategoryObject.Get()))
			{
				if(Struct->IsChildOf(FPoseLinkBase::StaticStruct()))
				{
					GetLinkedUTNodes_TraversePin(Pin, LinkedUTNodes);
				}
			}
		}
	}
}

void FUTBlueprintCompiler::GetLinkedUTNodes_TraversePin(UEdGraphPin* InPin, TArray<UUTGraphNode_Base*>& LinkedUTNodes)
{
	if(!InPin)
	{
		return;
	}

	for(UEdGraphPin* LinkedPin : InPin->LinkedTo)
	{
		if(!LinkedPin)
		{
			continue;
		}
		
		UEdGraphNode* OwningNode = LinkedPin->GetOwningNode();

		if(UK2Node_Knot* InnerKnot = Cast<UK2Node_Knot>(OwningNode))
		{
			GetLinkedUTNodes_TraversePin(InnerKnot->GetInputPin(), LinkedUTNodes);
		}
		else if(UUTGraphNode_Base* UTNode = Cast<UUTGraphNode_Base>(OwningNode))
		{
			GetLinkedUTNodes_ProcessUTNode(UTNode, LinkedUTNodes);
		}
	}
}

void FUTBlueprintCompiler::GetLinkedUTNodes_ProcessUTNode(UUTGraphNode_Base* UTNode, TArray<UUTGraphNode_Base *> &LinkedUTNodes)
{
	if(!AllocatedUTNodes.Contains(UTNode))
	{
		UUTGraphNode_Base* TrueSourceNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(AnimNode);

		if(UUTGraphNode_Base** AllocatedNode = SourceNodeToProcessedNodeMap.Find(TrueSourceNode))
		{
			LinkedUTNodes.Add(*AllocatedNode);
		}
		else
		{
			FString ErrorString = FString::Printf(*LOCTEXT("MissingLink", "Missing allocated node for %s while searching for node links - likely due to the node having outstanding errors.").ToString(), *AnimNode->GetName());
			MessageLog.Error(*ErrorString);
		}
	}
	else
	{
		LinkedAnimNodes.Add(AnimNode);
	}
}

void FUTBlueprintCompiler::ProcessAllUtilityTreeNodes()
{
	// Validate the graph
	ValidateGraphIsWellFormed(ConsolidatedEventGraph);

	// Validate that we have a skeleton
	if ((UtilityTreeBlueprint->TargetSkeleton == nullptr) && !UtilityTreeBlueprint->bIsNewlyCreated)
	{
		MessageLog.Error(*LOCTEXT("NoSkeleton", "@@ - The skeleton asset for this animation Blueprint is missing, so it cannot be compiled!").ToString(), UtilityTreeBlueprint);
		return;
	}

	// Build the raw node list
	TArray<UUTGraphNode_Base*> AnimNodeList;
	ConsolidatedEventGraph->GetNodesOfClass<UUTGraphNode_Base>(/*out*/ AnimNodeList);

	TArray<UK2Node_TransitionRuleGetter*> Getters;
	ConsolidatedEventGraph->GetNodesOfClass<UK2Node_TransitionRuleGetter>(/*out*/ Getters);

	// Get anim getters from the root anim graph (processing the nodes below will collect them in nested graphs)
	TArray<UK2Node_AnimGetter*> RootGraphAnimGetters;
	ConsolidatedEventGraph->GetNodesOfClass<UK2Node_AnimGetter>(RootGraphAnimGetters);

	// Find the root node
	UUTGraphNode_Root* PrePhysicsRoot = NULL;
	TArray<UUTGraphNode_Base*> RootSet;

	AllocateNodeIndexCounter = 0;
	NewUtilityTreeBlueprintClass->RootAnimNodeIndex = 0;//INDEX_NONE;

	for (auto SourceNodeIt = AnimNodeList.CreateIterator(); SourceNodeIt; ++SourceNodeIt)
	{
		UUTGraphNode_Base* SourceNode = *SourceNodeIt;
		UUTGraphNode_Base* TrueNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(SourceNode);
		TrueNode->BlueprintUsage = EBlueprintUsage::NoProperties;

		if (UUTGraphNode_Root* PossibleRoot = Cast<UUTGraphNode_Root>(SourceNode))
		{
			if (UUTGraphNode_Root* Root = ExactCast<UUTGraphNode_Root>(PossibleRoot))
			{
				if (PrePhysicsRoot != NULL)
				{
					MessageLog.Error(*FString::Printf(*LOCTEXT("ExpectedOneFunctionEntry_Error", "Expected only one animation root, but found both @@ and @@").ToString()),
						PrePhysicsRoot, Root);
				}
				else
				{
					RootSet.Add(Root);
					PrePhysicsRoot = Root;
				}
			}
		}
	}

	if (PrePhysicsRoot != NULL)
	{
		// Process the animation nodes
		ProcessUTNodesGivenRoot(UTNodeList, RootSet);

		// Process the getter nodes in the graph if there were any
		for (auto GetterIt = Getters.CreateIterator(); GetterIt; ++GetterIt)
		{
			ProcessTransitionGetter(*GetterIt, NULL); // transition nodes should not appear at top-level
		}

		// Wire root getters
		for(UK2Node_UTGetter* RootGraphGetter : RootGraphAnimGetters)
		{
			AutoWireUTGetter(RootGraphGetter, nullptr);
		}

		// Wire nested getters
		for(UK2Node_UTGetter* Getter : FoundGetterNodes)
		{
			AutoWireUTGetter(Getter, nullptr);
		}

		NewUtilityTreeBlueprintClass->RootAnimNodeIndex = GetAllocationIndexOfNode(PrePhysicsRoot);
	}
	else
	{
		MessageLog.Error(*FString::Printf(*LOCTEXT("ExpectedAFunctionEntry_Error", "Expected an animation root, but did not find one").ToString()));
	}
}

int32 FUTBlueprintCompiler::ExpandGraphAndProcessNodes(UEdGraph* SourceGraph, UUTGraphNode_Base* SourceRootNode, UAnimStateTransitionNode* TransitionNode, TArray<UEdGraphNode*>* ClonedNodes)
{
	// Clone the nodes from the source graph
	UEdGraph* ClonedGraph = FEdGraphUtilities::CloneGraph(SourceGraph, NULL, &MessageLog, true);

	// Grab all the animation nodes and find the corresponding root node in the cloned set
	UUTGraphNode_Base* TargetRootNode = NULL;
	TArray<UUTGraphNode_Base*> AnimNodeList;
	TArray<UK2Node_TransitionRuleGetter*> Getters;
	TArray<UK2Node_UTGetter*> AnimGetterNodes;

	for (auto NodeIt = ClonedGraph->Nodes.CreateIterator(); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = *NodeIt;

		if (UK2Node_TransitionRuleGetter* GetterNode = Cast<UK2Node_TransitionRuleGetter>(Node))
		{
			Getters.Add(GetterNode);
		}
		else if(UK2Node_AnimGetter* NewGetterNode = Cast<UK2Node_AnimGetter>(Node))
		{
			AnimGetterNodes.Add(NewGetterNode);
		}
		else if (UUTGraphNode_Base* TestNode = Cast<UUTGraphNode_Base>(Node))
		{
			AnimNodeList.Add(TestNode);

			//@TODO: There ought to be a better way to determine this
			if (MessageLog.FindSourceObject(TestNode) == MessageLog.FindSourceObject(SourceRootNode))
			{
				TargetRootNode = TestNode;
			}
		}

		if (ClonedNodes != NULL)
		{
			ClonedNodes->Add(Node);
		}
	}
	check(TargetRootNode);

	// Move the cloned nodes into the consolidated event graph
	const bool bIsLoading = Blueprint->bIsRegeneratingOnLoad || IsAsyncLoading();
	const bool bIsCompiling = Blueprint->bBeingCompiled;
	ClonedGraph->MoveNodesToAnotherGraph(ConsolidatedEventGraph, bIsLoading, bIsCompiling);

	// Process any animation nodes
	{
		TArray<UUTGraphNode_Base*> RootSet;
		RootSet.Add(TargetRootNode);
		ProcessUTNodesGivenRoot(AnimNodeList, RootSet);
	}

	// Process the getter nodes in the graph if there were any
	for (auto GetterIt = Getters.CreateIterator(); GetterIt; ++GetterIt)
	{
		ProcessTransitionGetter(*GetterIt, TransitionNode);
	}

	// Wire anim getter nodes
	for(UK2Node_AnimGetter* GetterNode : AnimGetterNodes)
	{
		FoundGetterNodes.Add(GetterNode);
	}

	// Returns the index of the processed cloned version of SourceRootNode
	return GetAllocationIndexOfNode(TargetRootNode);	
}

void FUTBlueprintCompiler::ProcessStateMachine(UUTGraphNode_StateMachineBase* StateMachineInstance)
{
	struct FMachineCreator
	{
	public:
		int32 MachineIndex;
		TMap<UAnimStateNodeBase*, int32> StateIndexTable;
		TMap<UAnimStateTransitionNode*, int32> TransitionIndexTable;
		UUtilityTreeBlueprintGeneratedClass* UtilityTreeBlueprintClass;
		UUTGraphNode_StateMachineBase* StateMachineInstance;
		FCompilerResultsLog& MessageLog;
	public:
		FMachineCreator(FCompilerResultsLog& InMessageLog, UUTGraphNode_StateMachineBase* InStateMachineInstance, int32 InMachineIndex, UUtilityTreeBlueprintGeneratedClass* InNewClass)
			: MachineIndex(InMachineIndex)
			, UtilityTreeBlueprintClass(InNewClass)
			, StateMachineInstance(InStateMachineInstance)
			, MessageLog(InMessageLog)
		{
			FStateMachineDebugData& MachineInfo = GetMachineSpecificDebugData();
			MachineInfo.MachineIndex = MachineIndex;
			MachineInfo.MachineInstanceNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_StateMachineBase>(InStateMachineInstance);

			StateMachineInstance->GetNode().StateMachineIndexInClass = MachineIndex;

			FBakedAnimationStateMachine& BakedMachine = GetMachine();
			BakedMachine.MachineName = StateMachineInstance->EditorStateMachineGraph->GetFName();
			BakedMachine.InitialState = INDEX_NONE;
		}

		FBakedAnimationStateMachine& GetMachine()
		{
			return UtilityTreeBlueprintClass->BakedStateMachines[MachineIndex];
		}

		FStateMachineDebugData& GetMachineSpecificDebugData()
		{
			UAnimationStateMachineGraph* SourceGraph = MessageLog.FindSourceObjectTypeChecked<UAnimationStateMachineGraph>(StateMachineInstance->EditorStateMachineGraph);
			return UtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.FindOrAdd(SourceGraph);
		}

		int32 FindOrAddState(UAnimStateNodeBase* StateNode)
		{
			if (int32* pResult = StateIndexTable.Find(StateNode))
			{
				return *pResult;
			}
			else
			{
				FBakedAnimationStateMachine& BakedMachine = GetMachine();

				const int32 StateIndex = BakedMachine.States.Num();
				StateIndexTable.Add(StateNode, StateIndex);
				new (BakedMachine.States) FBakedAnimationState();

				UAnimStateNodeBase* SourceNode = MessageLog.FindSourceObjectTypeChecked<UAnimStateNodeBase>(StateNode);
				GetMachineSpecificDebugData().NodeToStateIndex.Add(SourceNode, StateIndex);
				if (UAnimStateNode* SourceStateNode = Cast<UAnimStateNode>(SourceNode))
				{
					UtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateGraphToNodeMap.Add(SourceStateNode->BoundGraph, SourceStateNode);
				}

				return StateIndex;
			}
		}

		int32 FindOrAddTransition(UAnimStateTransitionNode* TransitionNode)
		{
			if (int32* pResult = TransitionIndexTable.Find(TransitionNode))
			{
				return *pResult;
			}
			else
			{
				FBakedAnimationStateMachine& BakedMachine = GetMachine();

				const int32 TransitionIndex = BakedMachine.Transitions.Num();
				TransitionIndexTable.Add(TransitionNode, TransitionIndex);
				new (BakedMachine.Transitions) FAnimationTransitionBetweenStates();

				UAnimStateTransitionNode* SourceTransitionNode = MessageLog.FindSourceObjectTypeChecked<UAnimStateTransitionNode>(TransitionNode);
				GetMachineSpecificDebugData().NodeToTransitionIndex.Add(SourceTransitionNode, TransitionIndex);
				UtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().TransitionGraphToNodeMap.Add(SourceTransitionNode->BoundGraph, SourceTransitionNode);

				if (SourceTransitionNode->CustomTransitionGraph != NULL)
				{
					UtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().TransitionBlendGraphToNodeMap.Add(SourceTransitionNode->CustomTransitionGraph, SourceTransitionNode);
				}

				return TransitionIndex;
			}
		}

		void Validate()
		{
			FBakedAnimationStateMachine& BakedMachine = GetMachine();

			// Make sure there is a valid entry point
			if (BakedMachine.InitialState == INDEX_NONE)
			{
				MessageLog.Warning(*LOCTEXT("NoEntryNode", "There was no entry state connection in @@").ToString(), StateMachineInstance);
				BakedMachine.InitialState = 0;
			}
			else
			{
				// Make sure the entry node is a state and not a conduit
				if (BakedMachine.States[BakedMachine.InitialState].bIsAConduit)
				{
					UEdGraphNode* StateNode = GetMachineSpecificDebugData().FindNodeFromStateIndex(BakedMachine.InitialState);
					MessageLog.Error(*LOCTEXT("BadStateEntryNode", "A conduit (@@) cannot be used as the entry node for a state machine").ToString(), StateNode);
				}
			}
		}
	};
	
	if (StateMachineInstance->EditorStateMachineGraph == NULL)
	{
		MessageLog.Error(*LOCTEXT("BadStateMachineNoGraph", "@@ does not have a corresponding graph").ToString(), StateMachineInstance);
		return;
	}

	TMap<UUTGraphNode_TransitionResult*, int32> AlreadyMergedTransitionList;

	const int32 MachineIndex = NewUtilityTreeBlueprintClass->BakedStateMachines.Num();
	new (NewUtilityTreeBlueprintClass->BakedStateMachines) FBakedAnimationStateMachine();
	FMachineCreator Oven(MessageLog, StateMachineInstance, MachineIndex, NewUtilityTreeBlueprintClass);

	// Map of states that contain a single player node (from state root node index to associated sequence player)
	TMap<int32, UObject*> SimplePlayerStatesMap;

	// Process all the states/transitions
	for (auto StateNodeIt = StateMachineInstance->EditorStateMachineGraph->Nodes.CreateIterator(); StateNodeIt; ++StateNodeIt)
	{
		UEdGraphNode* Node = *StateNodeIt;

		if (UAnimStateEntryNode* EntryNode = Cast<UAnimStateEntryNode>(Node))
		{
			// Handle the state graph entry
			FBakedAnimationStateMachine& BakedMachine = Oven.GetMachine();
			if (BakedMachine.InitialState != INDEX_NONE)
			{
				MessageLog.Error(*LOCTEXT("TooManyStateMachineEntryNodes", "Found an extra entry node @@").ToString(), EntryNode);
			}
			else if (UAnimStateNodeBase* StartState = Cast<UAnimStateNodeBase>(EntryNode->GetOutputNode()))
			{
				BakedMachine.InitialState = Oven.FindOrAddState(StartState);
			}
			else
			{
				MessageLog.Warning(*LOCTEXT("NoConnection", "Entry node @@ is not connected to state").ToString(), EntryNode);
			}
		}
		else if (UAnimStateTransitionNode* TransitionNode = Cast<UAnimStateTransitionNode>(Node))
		{
			TransitionNode->ValidateNodeDuringCompilation(MessageLog);

			const int32 TransitionIndex = Oven.FindOrAddTransition(TransitionNode);
			FAnimationTransitionBetweenStates& BakedTransition = Oven.GetMachine().Transitions[TransitionIndex];

			BakedTransition.CrossfadeDuration = TransitionNode->CrossfadeDuration;
			BakedTransition.StartNotify = FindOrAddNotify(TransitionNode->TransitionStart);
			BakedTransition.EndNotify = FindOrAddNotify(TransitionNode->TransitionEnd);
			BakedTransition.InterruptNotify = FindOrAddNotify(TransitionNode->TransitionInterrupt);
			BakedTransition.BlendMode = TransitionNode->BlendMode;
			BakedTransition.CustomCurve = TransitionNode->CustomBlendCurve;
			BakedTransition.BlendProfile = TransitionNode->BlendProfile;
			BakedTransition.LogicType = TransitionNode->LogicType;

			UAnimStateNodeBase* PreviousState = TransitionNode->GetPreviousState();
			UAnimStateNodeBase* NextState = TransitionNode->GetNextState();

			if ((PreviousState != NULL) && (NextState != NULL))
			{
				const int32 PreviousStateIndex = Oven.FindOrAddState(PreviousState);
				const int32 NextStateIndex = Oven.FindOrAddState(NextState);

				if (TransitionNode->Bidirectional)
				{
					MessageLog.Warning(*LOCTEXT("BidirectionalTransWarning", "Bidirectional transitions aren't supported yet @@").ToString(), TransitionNode);
				}

				BakedTransition.PreviousState = PreviousStateIndex;
				BakedTransition.NextState = NextStateIndex;
			}
			else
			{
				MessageLog.Warning(*LOCTEXT("BogusTransition", "@@ is incomplete, without a previous and next state").ToString(), TransitionNode);
				BakedTransition.PreviousState = INDEX_NONE;
				BakedTransition.NextState = INDEX_NONE;
			}
		}
		else if (UAnimStateNode* StateNode = Cast<UAnimStateNode>(Node))
		{
			StateNode->ValidateNodeDuringCompilation(MessageLog);

			const int32 StateIndex = Oven.FindOrAddState(StateNode);
			FBakedAnimationState& BakedState = Oven.GetMachine().States[StateIndex];

			if (StateNode->BoundGraph != NULL)
			{
				BakedState.StateName = StateNode->BoundGraph->GetFName();
				BakedState.StartNotify = FindOrAddNotify(StateNode->StateEntered);
				BakedState.EndNotify = FindOrAddNotify(StateNode->StateLeft);
				BakedState.FullyBlendedNotify = FindOrAddNotify(StateNode->StateFullyBlended);
				BakedState.bIsAConduit = false;
				BakedState.bAlwaysResetOnEntry = StateNode->bAlwaysResetOnEntry;

				// Process the inner graph of this state
				if (UUTGraphNode_StateResult* AnimGraphResultNode = CastChecked<UAnimationStateGraph>(StateNode->BoundGraph)->GetResultNode())
				{
					BakedState.StateRootNodeIndex = ExpandGraphAndProcessNodes(StateNode->BoundGraph, AnimGraphResultNode);

					// See if the state consists of a single sequence player node, and remember the index if so
					for (UEdGraphPin* TestPin : AnimGraphResultNode->Pins)
					{
						if ((TestPin->Direction == EGPD_Input) && (TestPin->LinkedTo.Num() == 1))
						{
							if (UUTGraphNode_SequencePlayer* SequencePlayer = Cast<UUTGraphNode_SequencePlayer>(TestPin->LinkedTo[0]->GetOwningNode()))
							{
								SimplePlayerStatesMap.Add(BakedState.StateRootNodeIndex, MessageLog.FindSourceObject(SequencePlayer));
							}
						}
					}
				}
				else
				{
					BakedState.StateRootNodeIndex = INDEX_NONE;
					MessageLog.Error(*LOCTEXT("StateWithNoResult", "@@ has no result node").ToString(), StateNode);
				}
			}
			else
			{
				BakedState.StateName = NAME_None;
				MessageLog.Error(*LOCTEXT("StateWithBadGraph", "@@ has no bound graph").ToString(), StateNode);
			}

			// If this check fires, then something in the machine has changed causing the states array to not
			// be a separate allocation, and a state machine inside of this one caused stuff to shift around
			checkSlow(&BakedState == &(Oven.GetMachine().States[StateIndex]));
		}
		else if (UAnimStateConduitNode* ConduitNode = Cast<UAnimStateConduitNode>(Node))
		{
			ConduitNode->ValidateNodeDuringCompilation(MessageLog);

			const int32 StateIndex = Oven.FindOrAddState(ConduitNode);
			FBakedAnimationState& BakedState = Oven.GetMachine().States[StateIndex];

			BakedState.StateName = ConduitNode->BoundGraph ? ConduitNode->BoundGraph->GetFName() : TEXT("OLD CONDUIT");
			BakedState.bIsAConduit = true;
			
			if (ConduitNode->BoundGraph != NULL)
			{
				if (UUTGraphNode_TransitionResult* EntryRuleResultNode = CastChecked<UAnimationTransitionGraph>(ConduitNode->BoundGraph)->GetResultNode())
				{
					BakedState.EntryRuleNodeIndex = ExpandGraphAndProcessNodes(ConduitNode->BoundGraph, EntryRuleResultNode);
				}
			}

			// If this check fires, then something in the machine has changed causing the states array to not
			// be a separate allocation, and a state machine inside of this one caused stuff to shift around
			checkSlow(&BakedState == &(Oven.GetMachine().States[StateIndex]));
		}
	}

	// Process transitions after all the states because getters within custom graphs may want to
	// reference back to other states, which are only valid if they have already been baked
	for (auto StateNodeIt = Oven.StateIndexTable.CreateIterator(); StateNodeIt; ++StateNodeIt)
	{
		UAnimStateNodeBase* StateNode = StateNodeIt.Key();
		const int32 StateIndex = StateNodeIt.Value();

		FBakedAnimationState& BakedState = Oven.GetMachine().States[StateIndex];

		// Add indices to all player nodes
		TArray<UEdGraph*> GraphsToCheck;
		TArray<UUTGraphNode_AssetPlayerBase*> AssetPlayerNodes;
		GraphsToCheck.Add(StateNode->GetBoundGraph());
		StateNode->GetBoundGraph()->GetAllChildrenGraphs(GraphsToCheck);

		for(UEdGraph* ChildGraph : GraphsToCheck)
		{
			ChildGraph->GetNodesOfClass(AssetPlayerNodes);
		}

		for(UUTGraphNode_AssetPlayerBase* Node : AssetPlayerNodes)
		{
			if(int32* IndexPtr = NewUtilityTreeBlueprintClass->UtilityTreeBlueprintDebugData.NodeGuidToIndexMap.Find(Node->NodeGuid))
			{
				BakedState.PlayerNodeIndices.Add(*IndexPtr);
			}
		}

		// Handle all the transitions out of this node
		TArray<class UAnimStateTransitionNode*> TransitionList;
		StateNode->GetTransitionList(/*out*/ TransitionList, /*bWantSortedList=*/ true);

		for (auto TransitionIt = TransitionList.CreateIterator(); TransitionIt; ++TransitionIt)
		{
			UAnimStateTransitionNode* TransitionNode = *TransitionIt;
			const int32 TransitionIndex = Oven.FindOrAddTransition(TransitionNode);

			FBakedStateExitTransition& Rule = *new (BakedState.Transitions) FBakedStateExitTransition();
			Rule.bDesiredTransitionReturnValue = (TransitionNode->GetPreviousState() == StateNode);
			Rule.TransitionIndex = TransitionIndex;
			
			if (UUTGraphNode_TransitionResult* TransitionResultNode = CastChecked<UAnimationTransitionGraph>(TransitionNode->BoundGraph)->GetResultNode())
			{
				if (int32* pIndex = AlreadyMergedTransitionList.Find(TransitionResultNode))
				{
					Rule.CanTakeDelegateIndex = *pIndex;
				}
				else
				{
					Rule.CanTakeDelegateIndex = ExpandGraphAndProcessNodes(TransitionNode->BoundGraph, TransitionResultNode, TransitionNode);
					AlreadyMergedTransitionList.Add(TransitionResultNode, Rule.CanTakeDelegateIndex);
				}
			}
			else
			{
				Rule.CanTakeDelegateIndex = INDEX_NONE;
				MessageLog.Error(*LOCTEXT("TransitionWithNoResult", "@@ has no result node").ToString(), TransitionNode);
			}

			// Handle automatic time remaining rules
			Rule.bAutomaticRemainingTimeRule = TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState;

			// Handle custom transition graphs
			Rule.CustomResultNodeIndex = INDEX_NONE;
			if (UAnimationCustomTransitionGraph* CustomTransitionGraph = Cast<UAnimationCustomTransitionGraph>(TransitionNode->CustomTransitionGraph))
			{
				TArray<UEdGraphNode*> ClonedNodes;
				if (CustomTransitionGraph->GetResultNode())
				{
					Rule.CustomResultNodeIndex = ExpandGraphAndProcessNodes(TransitionNode->CustomTransitionGraph, CustomTransitionGraph->GetResultNode(), NULL, &ClonedNodes);
				}

				// Find all the pose evaluators used in this transition, save handles to them because we need to populate some pose data before executing
				TArray<UUTGraphNode_TransitionPoseEvaluator*> TransitionPoseList;
				for (auto ClonedNodesIt = ClonedNodes.CreateIterator(); ClonedNodesIt; ++ClonedNodesIt)
				{
					UEdGraphNode* Node = *ClonedNodesIt;
					if (UUTGraphNode_TransitionPoseEvaluator* TypedNode = Cast<UUTGraphNode_TransitionPoseEvaluator>(Node))
					{
						TransitionPoseList.Add(TypedNode);
					}
				}

				Rule.PoseEvaluatorLinks.Empty(TransitionPoseList.Num());

				for (auto TransitionPoseListIt = TransitionPoseList.CreateIterator(); TransitionPoseListIt; ++TransitionPoseListIt)
				{
					UUTGraphNode_TransitionPoseEvaluator* TransitionPoseNode = *TransitionPoseListIt;
					Rule.PoseEvaluatorLinks.Add( GetAllocationIndexOfNode(TransitionPoseNode) );
				}
			}
		}
	}

	Oven.Validate();
}

void FUTBlueprintCompiler::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	Super::CopyTermDefaultsToDefaultObject(DefaultObject);

	if (bIsDerivedUtilityTreeBlueprint)
	{
		// If we are a derived animation graph; apply any stored overrides.
		// Restore values from the root class to catch values where the override has been removed.
		UUTBlueprintGeneratedClass* RootAnimClass = NewUtilityTreeBlueprintClass;
		while(UUtilityTreeBlueprintGeneratedClass* NextClass = Cast<UUtilityTreeBlueprintGeneratedClass>(RootAnimClass->GetSuperClass()))
		{
			RootAnimClass = NextClass;
		}
		UObject* RootDefaultObject = RootAnimClass->GetDefaultObject();

		for (TFieldIterator<UProperty> It(RootAnimClass) ; It; ++It)
		{
			UProperty* RootProp = *It;

			if (UStructProperty* RootStructProp = Cast<UStructProperty>(RootProp))
			{
				if (RootStructProp->Struct->IsChildOf(FUTNode_Base::StaticStruct()))
				{
					UStructProperty* ChildStructProp = FindField<UStructProperty>(NewUtilityTreeBlueprintClass, *RootStructProp->GetName());
					check(ChildStructProp);
					uint8* SourcePtr = RootStructProp->ContainerPtrToValuePtr<uint8>(RootDefaultObject);
					uint8* DestPtr = ChildStructProp->ContainerPtrToValuePtr<uint8>(DefaultObject);
					check(SourcePtr && DestPtr);
					RootStructProp->CopyCompleteValue(DestPtr, SourcePtr);
				}
			}
		}

		// Patch the overridden values into the CDO

		TArray<FAnimParentNodeAssetOverride*> AssetOverrides;
		UtilityTreeBlueprint->GetAssetOverrides(AssetOverrides);
		for (FAnimParentNodeAssetOverride* Override : AssetOverrides)
		{
			if (Override->NewAsset)
			{
				FUTNode_Base* BaseNode = NewUtilityTreeBlueprintClass->GetPropertyInstance<FUTNode_Base>(DefaultObject, Override->ParentNodeGuid, EPropertySearchMode::Hierarchy);
				if (BaseNode)
				{
					BaseNode->OverrideAsset(Override->NewAsset);
				}
			}
		}

		return;
	}

	int32 LinkIndexCount = 0;
	TMap<UUTGraphNode_Base*, int32> LinkIndexMap;
	TMap<UUTGraphNode_Base*, uint8*> NodeBaseAddresses;

	// Initialize animation nodes from their templates
	for (TFieldIterator<UProperty> It(DefaultObject->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UProperty* TargetProperty = *It;

		if (UUTGraphNode_Base* VisualUTNode = AllocatedNodePropertiesToNodes.FindRef(TargetProperty))
		{
			const UStructProperty* SourceNodeProperty = VisualUTNode->GetFNodeProperty();
			check(SourceNodeProperty != NULL);
			check(CastChecked<UStructProperty>(TargetProperty)->Struct == SourceNodeProperty->Struct);

			uint8* DestinationPtr = TargetProperty->ContainerPtrToValuePtr<uint8>(DefaultObject);
			uint8* SourcePtr = SourceNodeProperty->ContainerPtrToValuePtr<uint8>(VisualUTNode);
			TargetProperty->CopyCompleteValue(DestinationPtr, SourcePtr);

			LinkIndexMap.Add(VisualUTNode, LinkIndexCount);
			NodeBaseAddresses.Add(VisualUTNode, DestinationPtr);
			++LinkIndexCount;
		}
	}

	// And wire up node links
	for (auto PoseLinkIt = ValidPoseLinkList.CreateIterator(); PoseLinkIt; ++PoseLinkIt)
	{
		FPoseLinkMappingRecord& Record = *PoseLinkIt;

		UUTGraphNode_Base* LinkingNode = Record.GetLinkingNode();
		UUTGraphNode_Base* LinkedNode = Record.GetLinkedNode();
		
		// @TODO this is quick solution for crash - if there were previous errors and some nodes were not added, they could still end here -
		// this check avoids that and since there are already errors, compilation won't be successful.
		// but I'd prefer stoping compilation earlier to avoid getting here in first place
		if (LinkIndexMap.Contains(LinkingNode) && LinkIndexMap.Contains(LinkedNode))
		{
			const int32 SourceNodeIndex = LinkIndexMap.FindChecked(LinkingNode);
			const int32 LinkedNodeIndex = LinkIndexMap.FindChecked(LinkedNode);
			uint8* DestinationPtr = NodeBaseAddresses.FindChecked(LinkingNode);

			Record.PatchLinkIndex(DestinationPtr, LinkedNodeIndex, SourceNodeIndex);
		}
	}   

	// And patch evaluation function entry names
	for (auto EvalLinkIt = ValidEvaluationHandlerList.CreateIterator(); EvalLinkIt; ++EvalLinkIt)
	{
		FEvaluationHandlerRecord& Record = *EvalLinkIt;

		// validate fast path copy records before patching
		Record.ValidateFastPath(DefaultObject->GetClass());

		// patch either fast-path copy records or generated function names into the CDO
		Record.PatchFunctionNameAndCopyRecordsInto(DefaultObject);
	}

	// And patch in constant values that don't need to be re-evaluated every frame
	for (auto LiteralLinkIt = ValidAnimNodePinConstants.CreateIterator(); LiteralLinkIt; ++LiteralLinkIt)
	{
		FEffectiveConstantRecord& ConstantRecord = *LiteralLinkIt;

		//const FString ArrayClause = (ConstantRecord.ArrayIndex != INDEX_NONE) ? FString::Printf(TEXT("[%d]"), ConstantRecord.ArrayIndex) : FString();
		//const FString ValueClause = ConstantRecord.LiteralSourcePin->GetDefaultAsString();
		//MessageLog.Note(*FString::Printf(TEXT("Want to set %s.%s%s to %s"), *ConstantRecord.NodeVariableProperty->GetName(), *ConstantRecord.ConstantProperty->GetName(), *ArrayClause, *ValueClause));

		if (!ConstantRecord.Apply(DefaultObject))
		{
			MessageLog.Error(TEXT("ICE: Failed to push literal value from @@ into CDO"), ConstantRecord.LiteralSourcePin);
		}
	}
}

// Merges in any all ubergraph pages into the gathering ubergraph
void FUTBlueprintCompiler::MergeUbergraphPagesIn(UEdGraph* Ubergraph)
{
	Super::MergeUbergraphPagesIn(Ubergraph);

	if (bIsDerivedUtilityTreeBlueprint)
	{
		// Skip any work related to an anim graph, it's all done by the parent class
	}
	else
	{
		// Move all animation graph nodes and associated pure logic chains into the consolidated event graph
		for (int32 i = 0; i < Blueprint->FunctionGraphs.Num(); ++i)
		{
			UEdGraph* SourceGraph = Blueprint->FunctionGraphs[i];

			if (SourceGraph->Schema->IsChildOf(UUtilityTreeGraphSchema::StaticClass()))
			{
				// Merge all the animation nodes, contents, etc... into the ubergraph
				UEdGraph* ClonedGraph = FEdGraphUtilities::CloneGraph(SourceGraph, NULL, &MessageLog, true);
				const bool bIsLoading = Blueprint->bIsRegeneratingOnLoad || IsAsyncLoading();
				const bool bIsCompiling = Blueprint->bBeingCompiled;
				ClonedGraph->MoveNodesToAnotherGraph(ConsolidatedEventGraph, bIsLoading, bIsCompiling);
			}
		}

		// Make sure we expand any split pins here before we process animation nodes.
		for (TArray<UEdGraphNode*>::TIterator NodeIt(ConsolidatedEventGraph->Nodes); NodeIt; ++NodeIt)
		{
			UK2Node* K2Node = Cast<UK2Node>(*NodeIt);
			if (K2Node != nullptr)
			{
				// We iterate the array in reverse so we can recombine split-pins (which modifies the pins array)
				for (int32 PinIndex = K2Node->Pins.Num() - 1; PinIndex >= 0; --PinIndex)
				{
					UEdGraphPin* Pin = K2Node->Pins[PinIndex];
					if (Pin->SubPins.Num() == 0)
					{
						continue;
					}

					K2Node->ExpandSplitPin(this, ConsolidatedEventGraph, Pin);
				}
			}
		}

		// Compile the utility tree graph
		ProcessAllUTNodes();
	}
}

void FUTBlueprintCompiler::ProcessOneFunctionGraph(UEdGraph* SourceGraph, bool bInternalFunction)
{
	if (SourceGraph->Schema->IsChildOf(UUtilityTreeGraphSchema::StaticClass()))
	{
		// Animation graph
		// Do nothing, as this graph has already been processed
	}
	else
	{
		// Let the regular K2 compiler handle this one
		Super::ProcessOneFunctionGraph(SourceGraph, bInternalFunction);
	}
}

void FUTBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if( TargetUClass && !((UObject*)TargetUClass)->IsA(UUTBlueprintGeneratedClass::StaticClass()) )
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = NULL;
	}
}

void FUTBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	NewUTBlueprintClass = FindObject<UUTBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if (NewUTBlueprintClass == NULL)
	{
		NewUTBlueprintClass = NewObject<UUTBlueprintGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		FBlueprintCompileReinstancer::Create(NewUTBlueprintClass);
	}
	NewClass = NewUTBlueprintClass;
}

void FUTBlueprintCompiler::OnNewClassSet(UBlueprintGeneratedClass* ClassToUse)
{
	NewUTBlueprintClass = CastChecked<UUTBlueprintGeneratedClass>(ClassToUse);
}

void FUTBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOldCDO)
{
	Super::CleanAndSanitizeClass(ClassToClean, InOldCDO);

	// Make sure our typed pointer is set
	check(ClassToClean == NewClass && NewUtilityTreeBlueprintClass == NewClass);

	NewUtilityTreeBlueprintClass->UtilityTreeBlueprintDebugData = FUtilityTreeBlueprintDebugData();

	// Reset the baked data
	//@TODO: Move this into PurgeClass
	NewUtilityTreeBlueprintClass->BakedStateMachines.Empty();
	NewUtilityTreeBlueprintClass->AnimNotifies.Empty();

	NewUtilityTreeBlueprintClass->RootAnimNodeIndex = INDEX_NONE;
	NewUtilityTreeBlueprintClass->RootAnimNodeProperty = NULL;
	NewUtilityTreeBlueprintClass->OrderedSavedPoseIndices.Empty();
	NewUtilityTreeBlueprintClass->AnimNodeProperties.Empty();

	// Copy over runtime data from the blueprint to the class
	NewUtilityTreeBlueprintClass->TargetSkeleton = UtilityTreeBlueprint->TargetSkeleton;

	UUtilityTreeBlueprint* RootAnimBP = UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(UtilityTreeBlueprint);
	bIsDerivedUtilityTreeBlueprint = RootAnimBP != NULL;

	// Copy up data from a parent anim blueprint
	if (bIsDerivedUtilityTreeBlueprint)
	{
		UUtilityTreeBlueprintGeneratedClass* RootAnimClass = CastChecked<UUtilityTreeBlueprintGeneratedClass>(RootAnimBP->GeneratedClass);

		NewUtilityTreeBlueprintClass->BakedStateMachines.Append(RootAnimClass->BakedStateMachines);
		NewUtilityTreeBlueprintClass->AnimNotifies.Append(RootAnimClass->AnimNotifies);
		NewUtilityTreeBlueprintClass->RootAnimNodeIndex = RootAnimClass->RootAnimNodeIndex;
		NewUtilityTreeBlueprintClass->OrderedSavedPoseIndices = RootAnimClass->OrderedSavedPoseIndices;
	}
}

void FUTBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	const UUtilityTreeBlueprint* PossibleRoot = UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(UtilityTreeBlueprint);
	const UUtilityTreeBlueprint* Src = PossibleRoot ? PossibleRoot : UtilityTreeBlueprint;

	UUTBlueprintGeneratedClass* UTBlueprintGeneratedClass = CastChecked<UUTBlueprintGeneratedClass>(Class);
	UTBlueprintGeneratedClass->SyncGroupNames.Reset();
	UTBlueprintGeneratedClass->SyncGroupNames.Reserve(Src->Groups.Num());
	for (const FAnimGroupInfo& GroupInfo : Src->Groups)
	{
		UTBlueprintGeneratedClass->SyncGroupNames.Add(GroupInfo.Name);
	}
	Super::FinishCompilingClass(Class);
}

void FUTBlueprintCompiler::PostCompile()
{
	Super::PostCompile();

	UUTBlueprintGeneratedClass* UTBlueprintGeneratedClass = CastChecked<UUTBlueprintGeneratedClass>(NewClass);

	UUtilityTree* DefaultUTInstance = CastChecked<UUtilityTree>(UTBlueprintGeneratedClass->GetDefaultObject());


	for (const FEffectiveConstantRecord& ConstantRecord : ValidAnimNodePinConstants)
	{
		UUTGraphNode_Base* Node = CastChecked<UUTGraphNode_Base>(ConstantRecord.LiteralSourcePin->GetOwningNode());
		UUTGraphNode_Base* TrueNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(Node);
		TrueNode->BlueprintUsage = EBlueprintUsage::DoesNotUseBlueprint;
	}

	for(const FEvaluationHandlerRecord& EvaluationHandler : ValidEvaluationHandlerList)
	{
		if(EvaluationHandler.ServicedProperties.Num() > 0)
		{
			const FUTNodeSinglePropertyHandler& Handler = EvaluationHandler.ServicedProperties.CreateConstIterator()->Value;
			check(Handler.CopyRecords.Num() > 0);
			check(Handler.CopyRecords[0].DestPin != nullptr);
			UUTGraphNode_Base* Node = CastChecked<UUTGraphNode_Base>(Handler.CopyRecords[0].DestPin->GetOwningNode());
			UUTGraphNode_Base* TrueNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(Node);	

			FExposedValueHandler* HandlerPtr = EvaluationHandler.EvaluationHandlerProperty->ContainerPtrToValuePtr<FExposedValueHandler>(EvaluationHandler.NodeVariableProperty->ContainerPtrToValuePtr<void>(DefaultAnimInstance));
			TrueNode->BlueprintUsage = HandlerPtr->BoundFunction != NAME_None ? EBlueprintUsage::UsesBlueprint : EBlueprintUsage::DoesNotUseBlueprint;

#if WITH_EDITORONLY_DATA // ANIMINST_PostCompileValidation
			const bool bWarnAboutBlueprintUsage = UtilityTreeBlueprint->bWarnAboutBlueprintUsage || DefaultAnimInstance->PCV_ShouldWarnAboutNodesNotUsingFastPath();
#else
			const bool bWarnAboutBlueprintUsage = UtilityTreeBlueprint->bWarnAboutBlueprintUsage;
#endif
			if (bWarnAboutBlueprintUsage && (TrueNode->BlueprintUsage == EBlueprintUsage::UsesBlueprint))
			{
				MessageLog.Warning(*LOCTEXT("BlueprintUsageWarning", "Node @@ uses Blueprint to update its values, access member variables directly or use a constant value for better performance.").ToString(), Node);
			}
		}
	}

	for (UPoseWatch* PoseWatch : UtilityTreeBlueprint->PoseWatches)
	{
		AnimationEditorUtils::SetPoseWatch(PoseWatch, UtilityTreeBlueprint);
	}

	// iterate all anim node and call PostCompile
	const USkeleton* CurrentSkeleton = UtilityTreeBlueprint->TargetSkeleton;
	for (UStructProperty* Property : TFieldRange<UStructProperty>(UTBlueprintGeneratedClass, EFieldIteratorFlags::IncludeSuper))
	{
		if (Property->Struct->IsChildOf(FUTNode_Base::StaticStruct()))
		{
			FUTNode_Base* AnimNode = Property->ContainerPtrToValuePtr<FUTNode_Base>(DefaultUTInstance);
			AnimNode->PostCompile(CurrentSkeleton);
		}
	}
}

void FUTBlueprintCompiler::CreateFunctionList()
{
	// (These will now be processed after uber graph merge)

	// Build the list of functions and do preprocessing on all of them
	Super::CreateFunctionList();
}

void FUTBlueprintCompiler::ProcessTransitionGetter(UK2Node_TransitionRuleGetter* Getter, UAnimStateTransitionNode* TransitionNode)
{
	// Get common elements for multiple getters
	UEdGraphPin* OutputPin = Getter->GetOutputPin();

	UEdGraphPin* SourceTimePin = NULL;
	UUtilityTree* UtilityTree = NULL;
	int32 PlayerNodeIndex = INDEX_NONE;

	if (UUTGraphNode_Base* SourcePlayerNode = Getter->AssociatedAnimAssetPlayerNode)
	{
		// This check should never fail as the source state is always processed first before handling it's rules
		UUTGraphNode_Base* TrueSourceNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(SourcePlayerNode);
		UUTGraphNode_Base* UndertypedPlayerNode = SourceNodeToProcessedNodeMap.FindRef(TrueSourceNode);

		if (UndertypedPlayerNode == NULL)
		{
			MessageLog.Error(TEXT("ICE: Player node @@ was not processed prior to handling a transition getter @@ that used it"), SourcePlayerNode, Getter);
			return;
		}

		// Make sure the node is still relevant
		UEdGraph* PlayerGraph = UndertypedPlayerNode->GetGraph();
		if (!PlayerGraph->Nodes.Contains(UndertypedPlayerNode))
		{
			MessageLog.Error(TEXT("@@ is not associated with a node in @@; please delete and recreate it"), Getter, PlayerGraph);
		}

		// Make sure the referenced AnimAsset player has been allocated
		PlayerNodeIndex = GetAllocationIndexOfNode(UndertypedPlayerNode);
		if (PlayerNodeIndex == INDEX_NONE)
		{
			MessageLog.Error(*LOCTEXT("BadAnimAssetNodeUsedInGetter", "@@ doesn't have a valid associated AnimAsset node.  Delete and recreate it").ToString(), Getter);
		}

		// Grab the AnimAsset, and time pin if needed
		UScriptStruct* TimePropertyInStructType = NULL;
		const TCHAR* TimePropertyName = NULL;
		if (UndertypedPlayerNode->DoesSupportTimeForTransitionGetter())
		{
			UtilityTree = UndertypedPlayerNode->GetUtilityTree();
			TimePropertyInStructType = UndertypedPlayerNode->GetTimePropertyStruct();
			TimePropertyName = UndertypedPlayerNode->GetTimePropertyName();
		}
		else
		{
			MessageLog.Error(TEXT("@@ is associated with @@, which is an unexpected type"), Getter, UndertypedPlayerNode);
		}

		bool bNeedTimePin = false;

		// Determine if we need to read the current time variable from the specified sequence player
		switch (Getter->GetterType)
		{
		case ETransitionGetter::AnimationAsset_GetCurrentTime:
		case ETransitionGetter::AnimationAsset_GetCurrentTimeFraction:
		case ETransitionGetter::AnimationAsset_GetTimeFromEnd:
		case ETransitionGetter::AnimationAsset_GetTimeFromEndFraction:
			bNeedTimePin = true;
			break;
		default:
			bNeedTimePin = false;
			break;
		}

		if (bNeedTimePin && (PlayerNodeIndex != INDEX_NONE) && (TimePropertyName != NULL) && (TimePropertyInStructType != NULL))
		{
			UProperty* NodeProperty = AllocatedPropertiesByIndex.FindChecked(PlayerNodeIndex);

			// Create a struct member read node to grab the current position of the sequence player node
			UK2Node_StructMemberGet* TimeReadNode = SpawnIntermediateNode<UK2Node_StructMemberGet>(Getter, ConsolidatedEventGraph);
			TimeReadNode->VariableReference.SetSelfMember(NodeProperty->GetFName());
			TimeReadNode->StructType = TimePropertyInStructType;

			TimeReadNode->AllocatePinsForSingleMemberGet(TimePropertyName);
			SourceTimePin = TimeReadNode->FindPinChecked(TimePropertyName);
		}
	}

	// Expand it out
	UK2Node_CallFunction* GetterHelper = NULL;
	switch (Getter->GetterType)
	{
	case ETransitionGetter::AnimationAsset_GetCurrentTime:
		if ((UtilityTree != NULL) && (SourceTimePin != NULL))
		{
			GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceAssetPlayerTime"));
			GetterHelper->FindPinChecked(TEXT("AssetPlayerIndex"))->DefaultValue = FString::FromInt(PlayerNodeIndex);
		}
		else
		{
			if (Getter->AssociatedAnimAssetPlayerNode)
			{
				MessageLog.Error(TEXT("Please replace @@ with Get Relevant Anim Time. @@ has no animation asset"), Getter, Getter->AssociatedAnimAssetPlayerNode);
			}
			else
			{
				MessageLog.Error(TEXT("@@ is not asscociated with an asset player"), Getter);
			}
		}
		break;
	case ETransitionGetter::AnimationAsset_GetLength:
		if (UtilityTree != NULL)
		{
			GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceAssetPlayerLength"));
			GetterHelper->FindPinChecked(TEXT("AssetPlayerIndex"))->DefaultValue = FString::FromInt(PlayerNodeIndex);
		}
		else
		{
			if (Getter->AssociatedAnimAssetPlayerNode)
			{
				MessageLog.Error(TEXT("Please replace @@ with Get Relevant Anim Length. @@ has no animation asset"), Getter, Getter->AssociatedAnimAssetPlayerNode);
			}
			else
			{
				MessageLog.Error(TEXT("@@ is not asscociated with an asset player"), Getter);
			}
		}
		break;
	case ETransitionGetter::AnimationAsset_GetCurrentTimeFraction:
		if ((UtilityTree  != NULL) && (SourceTimePin != NULL))
		{
			GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceAssetPlayerTimeFraction"));
			GetterHelper->FindPinChecked(TEXT("AssetPlayerIndex"))->DefaultValue = FString::FromInt(PlayerNodeIndex);
		}
		else
		{
			if (Getter->AssociatedAnimAssetPlayerNode)
			{
				MessageLog.Error(TEXT("Please replace @@ with Get Relevant Anim Time Fraction. @@ has no animation asset"), Getter, Getter->AssociatedAnimAssetPlayerNode);
			}
			else
			{
				MessageLog.Error(TEXT("@@ is not asscociated with an asset player"), Getter);
			}
		}
		break;
	case ETransitionGetter::AnimationAsset_GetTimeFromEnd:
		if ((UtilityTree != NULL) && (SourceTimePin != NULL))
		{
			GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceAssetPlayerTimeFromEnd"));
			GetterHelper->FindPinChecked(TEXT("AssetPlayerIndex"))->DefaultValue = FString::FromInt(PlayerNodeIndex);
		}
		else
		{
			if (Getter->AssociatedAnimAssetPlayerNode)
			{
				MessageLog.Error(TEXT("Please replace @@ with Get Relevant Anim Time Remaining. @@ has no animation asset"), Getter, Getter->AssociatedAnimAssetPlayerNode);
			}
			else
			{
				MessageLog.Error(TEXT("@@ is not asscociated with an asset player"), Getter);
			}
		}
		break;
	case ETransitionGetter::AnimationAsset_GetTimeFromEndFraction:
		if ((UtilityTree != NULL) && (SourceTimePin != NULL))
		{
			GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceAssetPlayerTimeFromEndFraction"));
			GetterHelper->FindPinChecked(TEXT("AssetPlayerIndex"))->DefaultValue = FString::FromInt(PlayerNodeIndex);
		}
		else
		{
			if (Getter->AssociatedAnimAssetPlayerNode)
			{
				MessageLog.Error(TEXT("Please replace @@ with Get Relevant Anim Time Remaining Fraction. @@ has no animation asset"), Getter, Getter->AssociatedAnimAssetPlayerNode);
			}
			else
			{
				MessageLog.Error(TEXT("@@ is not asscociated with an asset player"), Getter);
			}
		}
		break;

	case ETransitionGetter::CurrentTransitionDuration:
		{
			check(TransitionNode);
			if(UAnimStateNode* SourceStateNode = MessageLog.FindSourceObjectTypeChecked<UAnimStateNode>(TransitionNode->GetPreviousState()))
			{
				if(UObject* SourceTransitionNode = MessageLog.FindSourceObject(TransitionNode))
				{
					if(FStateMachineDebugData* DebugData = NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.Find(SourceStateNode->GetGraph()))
					{
						if(int32* pStateIndex = DebugData->NodeToStateIndex.Find(SourceStateNode))
						{
							const int32 StateIndex = *pStateIndex;
							
							// This check should never fail as all Utility Tree nodes should be processed before getters are
							UUTGraphNode_Base* CompiledMachineInstanceNode = SourceNodeToProcessedNodeMap.FindChecked(DebugData->MachineInstanceNode.Get());
							const int32 MachinePropertyIndex = AllocatedAnimNodeIndices.FindChecked(CompiledMachineInstanceNode);
							int32 TransitionPropertyIndex = INDEX_NONE;

							for(TMap<TWeakObjectPtr<UEdGraphNode>, int32>::TIterator TransIt(DebugData->NodeToTransitionIndex); TransIt; ++TransIt)
							{
								UEdGraphNode* CurrTransNode = TransIt.Key().Get();
								
								if(CurrTransNode == SourceTransitionNode)
								{
									TransitionPropertyIndex = TransIt.Value();
									break;
								}
							}

							if(TransitionPropertyIndex != INDEX_NONE)
							{
								GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceTransitionCrossfadeDuration"));
								GetterHelper->FindPinChecked(TEXT("MachineIndex"))->DefaultValue = FString::FromInt(MachinePropertyIndex);
								GetterHelper->FindPinChecked(TEXT("TransitionIndex"))->DefaultValue = FString::FromInt(TransitionPropertyIndex);
							}
						}
					}
				}
			}
		}
		break;

	case ETransitionGetter::ArbitraryState_GetBlendWeight:
		{
			if (Getter->AssociatedStateNode)
			{
				if (UAnimStateNode* SourceStateNode = MessageLog.FindSourceObjectTypeChecked<UAnimStateNode>(Getter->AssociatedStateNode))
				{
					if (FStateMachineDebugData* DebugData = NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.Find(SourceStateNode->GetGraph()))
					{
						if (int32* pStateIndex = DebugData->NodeToStateIndex.Find(SourceStateNode))
						{
							const int32 StateIndex = *pStateIndex;
							//const int32 MachineIndex = DebugData->MachineIndex;

							// This check should never fail as all Utility Tree nodes should be processed before getters are
							UUTGraphNode_Base* CompiledMachineInstanceNode = SourceNodeToProcessedNodeMap.FindChecked(DebugData->MachineInstanceNode.Get());
							const int32 MachinePropertyIndex = AllocatedAnimNodeIndices.FindChecked(CompiledMachineInstanceNode);

							GetterHelper = SpawnCallAnimInstanceFunction(Getter, TEXT("GetInstanceStateWeight"));
							GetterHelper->FindPinChecked(TEXT("MachineIndex"))->DefaultValue = FString::FromInt(MachinePropertyIndex);
							GetterHelper->FindPinChecked(TEXT("StateIndex"))->DefaultValue = FString::FromInt(StateIndex);
						}
					}
				}
			}

			if (GetterHelper == NULL)
			{
				MessageLog.Error(TEXT("@@ is not associated with a valid state"), Getter);
			}
		}
		break;

	case ETransitionGetter::CurrentState_ElapsedTime:
		{
			check(TransitionNode);
			if (UAnimStateNode* SourceStateNode = MessageLog.FindSourceObjectTypeChecked<UAnimStateNode>(TransitionNode->GetPreviousState()))
			{
				if (FStateMachineDebugData* DebugData = NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.Find(SourceStateNode->GetGraph()))
				{
					// This check should never fail as all Utility Tree nodes should be processed before getters are
					UUTGraphNode_Base* CompiledMachineInstanceNode = SourceNodeToProcessedNodeMap.FindChecked(DebugData->MachineInstanceNode.Get());
					const int32 MachinePropertyIndex = AllocatedUTNodeIndices.FindChecked(CompiledMachineInstanceNode);

					GetterHelper = SpawnCallUTInstanceFunction(Getter, TEXT("GetInstanceCurrentStateElapsedTime"));
					GetterHelper->FindPinChecked(TEXT("MachineIndex"))->DefaultValue = FString::FromInt(MachinePropertyIndex);
				}
			}
			if (GetterHelper == NULL)
			{
				MessageLog.Error(TEXT("@@ is not associated with a valid state"), Getter);
			}
		}
		break;

	case ETransitionGetter::CurrentState_GetBlendWeight:
		{
			check(TransitionNode);
			if (UUTStateNode* SourceStateNode = MessageLog.FindSourceObjectTypeChecked<UAnimStateNode>(TransitionNode->GetPreviousState()))
			{
				{
					if (FStateMachineDebugData* DebugData = NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.Find(SourceStateNode->GetGraph()))
					{
						if (int32* pStateIndex = DebugData->NodeToStateIndex.Find(SourceStateNode))
						{
							const int32 StateIndex = *pStateIndex;
							//const int32 MachineIndex = DebugData->MachineIndex;

							// This check should never fail as all Utility Tree nodes should be processed before getters are
							UUTGraphNode_Base* CompiledMachineInstanceNode = SourceNodeToProcessedNodeMap.FindChecked(DebugData->MachineInstanceNode.Get());
							const int32 MachinePropertyIndex = AllocatedUTNodeIndices.FindChecked(CompiledMachineInstanceNode);

							GetterHelper = SpawnCallUTInstanceFunction(Getter, TEXT("GetInstanceStateWeight"));
							GetterHelper->FindPinChecked(TEXT("MachineIndex"))->DefaultValue = FString::FromInt(MachinePropertyIndex);
							GetterHelper->FindPinChecked(TEXT("StateIndex"))->DefaultValue = FString::FromInt(StateIndex);
						}
					}
				}
			}
			if (GetterHelper == NULL)
			{
				MessageLog.Error(TEXT("@@ is not associated with a valid state"), Getter);
			}
		}
		break;

	default:
		MessageLog.Error(TEXT("Unrecognized getter type on @@"), Getter);
		break;
	}

	// Finish wiring up a call function if needed
	if (GetterHelper != NULL)
	{
		check(GetterHelper->IsNodePure());

		UEdGraphPin* NewReturnPin = GetterHelper->FindPinChecked(TEXT("ReturnValue"));
		MessageLog.NotifyIntermediatePinCreation(NewReturnPin, OutputPin);

		NewReturnPin->CopyPersistentDataFromOldPin(*OutputPin);
	}

	// Remove the getter from the equation
	Getter->BreakAllNodeLinks();
}

void FUTBlueprintCompiler::PostCompileDiagnostics()
{
	FKismetCompilerContext::PostCompileDiagnostics();

#if WITH_EDITORONLY_DATA // ANIMINST_PostCompileValidation
	// See if AnimInstance implements a PostCompileValidation Class. 
	// If so, instantiate it, and let it perform Validation of our newly compiled UtilityTreeBlueprint.
	if (const UAnimInstance* const DefaultAnimInstance = CastChecked<UAnimInstance>(NewUtilityTreeBlueprintClass->GetDefaultObject()))
	{
		if (DefaultAnimInstance->PostCompileValidationClassName.IsValid())
		{
			UClass* PostCompileValidationClass = LoadClass<UObject>(nullptr, *DefaultAnimInstance->PostCompileValidationClassName.ToString());
			if (PostCompileValidationClass)
			{
				UUTBlueprintPostCompileValidation* PostCompileValidation = NewObject<UUTBlueprintPostCompileValidation>(GetTransientPackage(), PostCompileValidationClass);
				if (PostCompileValidation)
				{
					FAnimBPCompileValidationParams PCV_Params(DefaultAnimInstance, NewUtilityTreeBlueprintClass, MessageLog, AllocatedNodePropertiesToNodes);
					PostCompileValidation->DoPostCompileValidation(PCV_Params);
				}
			}
		}
	}
#endif // WITH_EDITORONLY_DATA

	if (!bIsDerivedUtilityTreeBlueprint)
	{
		// Run thru all nodes and make sure they like the final results
		for (auto NodeIt = AllocatedUTNodeIndices.CreateConstIterator(); NodeIt; ++NodeIt)
		{
			if (UUTGraphNode_Base* Node = NodeIt.Key())
			{
				Node->ValidateUTNodePostCompile(MessageLog, NewUtilityTreeBlueprintClass, NodeIt.Value());
			}
		}

		//
		bool bDisplayUTDebug = false;
		if (!Blueprint->bIsRegeneratingOnLoad)
		{
			GConfig->GetBool(TEXT("Kismet"), TEXT("CompileDisplaysUtilityTreeBlueprintBackend"), /*out*/ bDisplayUTDebug, GEngineIni);

			if (bDisplayUTDebug)
			{
				DumpUTDebugData();
			}
		}
	}
}

void FUTBlueprintCompiler::DumpUTDebugData()
{
	// List all compiled-down nodes and their sources
	if (NewUtilityTreeBlueprintClass->RootAnimNodeProperty == NULL)
	{
		return;
	}

	int32 RootIndex = INDEX_NONE;
	NewUtilityTreeBlueprintClass->AnimNodeProperties.Find(NewUtilityTreeBlueprintClass->RootAnimNodeProperty, /*out*/ RootIndex);
	
	uint8* CDOBase = (uint8*)NewUtilityTreeBlueprintClass->ClassDefaultObject;

	MessageLog.Note(*FString::Printf(TEXT("Anim Root is #%d"), RootIndex));
	for (int32 Index = 0; Index < NewUtilityTreeBlueprintClass->AnimNodeProperties.Num(); ++Index)
	{
		UStructProperty* NodeProperty = NewUtilityTreeBlueprintClass->AnimNodeProperties[Index];
		FString PropertyName = NodeProperty->GetName();
		FString PropertyType = NodeProperty->Struct->GetName();
		FString RootSuffix = (Index == RootIndex) ? TEXT(" <--- ROOT") : TEXT("");

		// Print out the node
		MessageLog.Note(*FString::Printf(TEXT("[%d] @@ [prop %s]%s"), Index, *PropertyName, *RootSuffix), AllocatedNodePropertiesToNodes.FindRef(NodeProperty));

		// Print out all the node links
		for (TFieldIterator<UProperty> PropIt(NodeProperty->Struct, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
		{
			UProperty* ChildProp = *PropIt;
			if (UStructProperty* ChildStructProp = Cast<UStructProperty>(ChildProp))
			{
				if (ChildStructProp->Struct->IsChildOf(FPoseLinkBase::StaticStruct()))
				{
					uint8* ChildPropertyPtr =  ChildStructProp->ContainerPtrToValuePtr<uint8>(NodeProperty->ContainerPtrToValuePtr<uint8>(CDOBase));
					FPoseLinkBase* ChildPoseLink = (FPoseLinkBase*)ChildPropertyPtr;

					if (ChildPoseLink->LinkID != INDEX_NONE)
					{
						UStructProperty* LinkedProperty = NewUtilityTreeBlueprintClass->AnimNodeProperties[ChildPoseLink->LinkID];
						MessageLog.Note(*FString::Printf(TEXT("   Linked via %s to [#%d] @@"), *ChildStructProp->GetName(), ChildPoseLink->LinkID), AllocatedNodePropertiesToNodes.FindRef(LinkedProperty));
					}
					else
					{
						MessageLog.Note(*FString::Printf(TEXT("   Linked via %s to <no connection>"), *ChildStructProp->GetName()));
					}
				}
			}
		}
	}

	const int32 foo = NewUtilityTreeBlueprintClass->AnimNodeProperties.Num() - 1;

	MessageLog.Note(TEXT("State machine info:"));
	for (int32 MachineIndex = 0; MachineIndex < NewUtilityTreeBlueprintClass->BakedStateMachines.Num(); ++MachineIndex)
	{
		FBakedAnimationStateMachine& Machine = NewUtilityTreeBlueprintClass->BakedStateMachines[MachineIndex];
	
		MessageLog.Note(*FString::Printf(TEXT("Machine %s starts at state #%d (%s) and has %d states, %d transitions"),
			*(Machine.MachineName.ToString()),
			Machine.InitialState,
			*(Machine.States[Machine.InitialState].StateName.ToString()),
			Machine.States.Num(),
			Machine.Transitions.Num()));

		for (int32 StateIndex = 0; StateIndex < Machine.States.Num(); ++StateIndex)
		{
			FBakedAnimationState& SingleState = Machine.States[StateIndex];
			
			MessageLog.Note(*FString::Printf(TEXT("  State #%d is named %s, with %d exit transitions; linked to graph #%d"),
				StateIndex,
				*(SingleState.StateName.ToString()),
				SingleState.Transitions.Num(),
				foo - SingleState.StateRootNodeIndex));

			for (int32 RuleIndex = 0; RuleIndex < SingleState.Transitions.Num(); ++RuleIndex)
			{
				FBakedStateExitTransition& ExitTransition = SingleState.Transitions[RuleIndex];

				int32 TargetStateIndex = Machine.Transitions[ExitTransition.TransitionIndex].NextState;

				MessageLog.Note(*FString::Printf(TEXT("    Exit trans #%d to %s uses global trans %d, wanting %s, linked to delegate #%d "),
					RuleIndex,
					*Machine.States[TargetStateIndex].StateName.ToString(),
					ExitTransition.TransitionIndex,
					ExitTransition.bDesiredTransitionReturnValue ? TEXT("TRUE") : TEXT("FALSE"),
					foo - ExitTransition.CanTakeDelegateIndex));
			}
		}
	}
}

void FUTBlueprintCompiler::AutoWireUTGetter(class UK2Node_UTGetter* Getter, UUTStateTransitionNode* InTransitionNode)
{
	UEdGraphPin* ReferencedNodeTimePin = nullptr;
	int32 ReferencedNodeIndex = INDEX_NONE;
	int32 SubNodeIndex = INDEX_NONE;
	
	UUTGraphNode_Base* ProcessedNodeCheck = NULL;

	if(UUTGraphNode_Base* SourceNode = Getter->SourceNode)
	{
		UUTGraphNode_Base* ActualSourceNode = MessageLog.FindSourceObjectTypeChecked<UUTGraphNode_Base>(SourceNode);
		
		if(UUTGraphNode_Base* ProcessedSourceNode = SourceNodeToProcessedNodeMap.FindRef(ActualSourceNode))
		{
			ProcessedNodeCheck = ProcessedSourceNode;

			ReferencedNodeIndex = GetAllocationIndexOfNode(ProcessedSourceNode);

			if(ProcessedSourceNode->DoesSupportTimeForTransitionGetter())
			{
				UScriptStruct* TimePropertyInStructType = ProcessedSourceNode->GetTimePropertyStruct();
				const TCHAR* TimePropertyName = ProcessedSourceNode->GetTimePropertyName();

				if(ReferencedNodeIndex != INDEX_NONE && TimePropertyName && TimePropertyInStructType)
				{
					UProperty* NodeProperty = AllocatedPropertiesByIndex.FindChecked(ReferencedNodeIndex);

					UK2Node_StructMemberGet* ReaderNode = SpawnIntermediateNode<UK2Node_StructMemberGet>(Getter, ConsolidatedEventGraph);
					ReaderNode->VariableReference.SetSelfMember(NodeProperty->GetFName());
					ReaderNode->StructType = TimePropertyInStructType;
					ReaderNode->AllocatePinsForSingleMemberGet(TimePropertyName);

					ReferencedNodeTimePin = ReaderNode->FindPinChecked(TimePropertyName);
				}
			}
		}
	}
	
	if(Getter->SourceStateNode)
	{
		UObject* SourceObject = MessageLog.FindSourceObject(Getter->SourceStateNode);
		if(UAnimStateNode* SourceStateNode = Cast<UAnimStateNode>(SourceObject))
		{
			if(FStateMachineDebugData* DebugData = NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.Find(SourceStateNode->GetGraph()))
			{
				if(int32* StateIndexPtr = DebugData->NodeToStateIndex.Find(SourceStateNode))
				{
					SubNodeIndex = *StateIndexPtr;
				}
			}
		}
		else if(UAnimStateTransitionNode* TransitionNode = Cast<UAnimStateTransitionNode>(SourceObject))
		{
			if(FStateMachineDebugData* DebugData = NewUtilityTreeBlueprintClass->GetUtilityTreeBlueprintDebugData().StateMachineDebugData.Find(TransitionNode->GetGraph()))
			{
				if(int32* TransitionIndexPtr = DebugData->NodeToTransitionIndex.Find(TransitionNode))
				{
					SubNodeIndex = *TransitionIndexPtr;
				}
			}
		}
	}

	check(Getter->IsNodePure());

	for(UEdGraphPin* Pin : Getter->Pins)
	{
		// Hook up autowired parameters / pins
		if(Pin->PinName == TEXT("CurrentTime"))
		{
			Pin->MakeLinkTo(ReferencedNodeTimePin);
		}
		else if(Pin->PinName == TEXT("AssetPlayerIndex") || Pin->PinName == TEXT("MachineIndex"))
		{
			Pin->DefaultValue = FString::FromInt(ReferencedNodeIndex);
		}
		else if(Pin->PinName == TEXT("StateIndex") || Pin->PinName == TEXT("TransitionIndex"))
		{
			Pin->DefaultValue = FString::FromInt(SubNodeIndex);
		}
	}
}

void FUTBlueprintCompiler::FEvaluationHandlerRecord::PatchFunctionNameAndCopyRecordsInto(UObject* TargetObject) const
{
	FExposedValueHandler* HandlerPtr = EvaluationHandlerProperty->ContainerPtrToValuePtr<FExposedValueHandler>(NodeVariableProperty->ContainerPtrToValuePtr<void>(TargetObject));
	HandlerPtr->CopyRecords.Empty();

	if (IsFastPath())
	{
		for (const TPair<FName, FUTNodeSinglePropertyHandler>& ServicedPropPair : ServicedProperties)
		{
			const FName& PropertyName = ServicedPropPair.Key;
			const FUTNodeSinglePropertyHandler& PropertyHandler = ServicedPropPair.Value;

			for (const FPropertyCopyRecord& PropertyCopyRecord : PropertyHandler.CopyRecords)
			{
				  // get the correct property sizes for the type we are dealing with (array etc.)
				  int32 DestPropertySize = PropertyCopyRecord.DestProperty->GetSize();
				  if (UArrayProperty* DestArrayProperty = Cast<UArrayProperty>(PropertyCopyRecord.DestProperty))
				  {
					  DestPropertySize = DestArrayProperty->Inner->GetSize();
				  }

				  FExposedValueCopyRecord CopyRecord;
				  CopyRecord.DestProperty = PropertyCopyRecord.DestProperty;
				  CopyRecord.DestArrayIndex = PropertyCopyRecord.DestArrayIndex == INDEX_NONE ? 0 : PropertyCopyRecord.DestArrayIndex;
				  CopyRecord.SourcePropertyName = PropertyCopyRecord.SourcePropertyName;
				  CopyRecord.SourceSubPropertyName = PropertyCopyRecord.SourceSubStructPropertyName;
				  CopyRecord.SourceArrayIndex = 0;
				  CopyRecord.Size = DestPropertySize;
				  CopyRecord.PostCopyOperation = PropertyCopyRecord.Operation;
				  CopyRecord.bInstanceIsTarget = PropertyHandler.bInstanceIsTarget;
				  HandlerPtr->CopyRecords.Add(CopyRecord);
			}
		}
	}
	else
	{
		// not all of our pins use copy records so we will need to call our exposed value handler
		HandlerPtr->BoundFunction = HandlerFunctionName;
	}
}

static UEdGraphPin* FindFirstInputPin(UEdGraphNode* InNode)
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();

	for(UEdGraphPin* Pin : InNode->Pins)
	{
		if(Pin && Pin->Direction == EGPD_Input && !Schema->IsExecPin(*Pin) && !Schema->IsSelfPin(*Pin))
		{
			return Pin;
		}
	}

	return nullptr;
}

static UEdGraphNode* FollowKnots(UEdGraphPin* FromPin, UEdGraphPin*& ToPin)
{
	if (FromPin->LinkedTo.Num() == 0)
	{
		return nullptr;
	}

	UEdGraphPin* LinkedPin = FromPin->LinkedTo[0];
	ToPin = LinkedPin;
	if(LinkedPin)
	{
		UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
		UK2Node_Knot* KnotNode = Cast<UK2Node_Knot>(LinkedNode);
		while(KnotNode)
		{
			if(UEdGraphPin* InputPin = FindFirstInputPin(KnotNode))
			{
				if (InputPin->LinkedTo.Num() > 0 && InputPin->LinkedTo[0])
				{
					ToPin = InputPin->LinkedTo[0];
					LinkedNode = InputPin->LinkedTo[0]->GetOwningNode();
					KnotNode = Cast<UK2Node_Knot>(LinkedNode);
				}
				else
				{
					KnotNode = nullptr;
				}
			}
		}
		return LinkedNode;
	}

	return nullptr;
}

void FUTBlueprintCompiler::FEvaluationHandlerRecord::RegisterPin(UEdGraphPin* DestPin, UProperty* AssociatedProperty, int32 AssociatedPropertyArrayIndex)
{
	FUTNodeSinglePropertyHandler& Handler = ServicedProperties.FindOrAdd(AssociatedProperty->GetFName());
	Handler.CopyRecords.Emplace(DestPin, AssociatedProperty, AssociatedPropertyArrayIndex);
}

void FUTBlueprintCompiler::FEvaluationHandlerRecord::BuildFastPathCopyRecords()
{
	if (GetDefault<UEngine>()->bOptimizeUtilityTreeBlueprintMemberVariableAccess)
	{
		for (TPair<FName, FUTNodeSinglePropertyHandler>& ServicedPropPair : ServicedProperties)
		{
			for (FPropertyCopyRecord& CopyRecord : ServicedPropPair.Value.CopyRecords)
			{
				typedef bool (FUTBlueprintCompiler::FEvaluationHandlerRecord::*GraphCheckerFunc)(FPropertyCopyRecord&, UEdGraphPin*);

				GraphCheckerFunc GraphCheckerFuncs[] =
				{
					&FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForVariableGet,
					&FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForLogicalNot,
					&FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForStructMemberAccess,
				};

				for (GraphCheckerFunc& CheckFunc : GraphCheckerFuncs)
				{
					if ((this->*CheckFunc)(CopyRecord, CopyRecord.DestPin))
					{
						break;
					}
				}

				CheckForMemberOnlyAccess(CopyRecord, CopyRecord.DestPin);
			}
		}
	}
}

static FName RecoverSplitStructPinName(UEdGraphPin* OutputPin)
{
	check(OutputPin->ParentPin);
	
	const FString& PinName = OutputPin->PinName;
	const FString& ParentPinName = OutputPin->ParentPin->PinName;

	return FName(*PinName.Replace(*(ParentPinName + TEXT("_")), TEXT("")));
}

bool FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForVariableGet(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin)
{
	if(DestPin)
	{
		UEdGraphPin* SourcePin = nullptr;
		if(UK2Node_VariableGet* VariableGetNode = Cast<UK2Node_VariableGet>(FollowKnots(DestPin, SourcePin)))
		{
			if(VariableGetNode && VariableGetNode->IsNodePure() && VariableGetNode->VariableReference.IsSelfContext())
			{
				if(SourcePin)
				{
					// variable get could be a 'split' struct
					if(SourcePin->ParentPin != nullptr)
					{
						CopyRecord.SourcePropertyName = VariableGetNode->VariableReference.GetMemberName();
						CopyRecord.SourceSubStructPropertyName = RecoverSplitStructPinName(SourcePin);
					}
					else
					{
						CopyRecord.SourcePropertyName = VariableGetNode->VariableReference.GetMemberName();
					}
					return true;
				}
			}
		}
	}

	return false;
}

bool FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForLogicalNot(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin)
{
	if(DestPin)
	{
		UEdGraphPin* SourcePin = nullptr;
		UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(FollowKnots(DestPin, SourcePin));
		if(CallFunctionNode && CallFunctionNode->FunctionReference.GetMemberName() == FName(TEXT("Not_PreBool")))
		{
			// find and follow input pin
			if(UEdGraphPin* InputPin = FindFirstInputPin(CallFunctionNode))
			{
				check(InputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean);
				if(CheckForVariableGet(CopyRecord, InputPin) || CheckForStructMemberAccess(CopyRecord, InputPin))
				{
					check(CopyRecord.SourcePropertyName != NAME_None);	// this should have been filled in by CheckForVariableGet() or CheckForStructMemberAccess() above
					CopyRecord.Operation = EPostCopyOperation::LogicalNegateBool;
					return true;
				}
			}
		}
	}

	return false;
}

/** The functions that we can safely native-break */
static const FName NativeBreakFunctionNameWhitelist[] =
{
	FName(TEXT("BreakVector")),
	FName(TEXT("BreakVector2D")),
	FName(TEXT("BreakRotator")),
};

/** Check whether a native break function can be safely used in the fast-path copy system (ie. source and dest data will be the same) */
static bool IsWhitelistedNativeBreak(const FName& InFunctionName)
{
	for(const FName& FunctionName : NativeBreakFunctionNameWhitelist)
	{
		if(InFunctionName == FunctionName)
		{
			return true;
		}
	}

	return false;
}

bool FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForStructMemberAccess(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin)
{
	if(DestPin)
	{
		UEdGraphPin* SourcePin = nullptr;
		if(UK2Node_BreakStruct* BreakStructNode = Cast<UK2Node_BreakStruct>(FollowKnots(DestPin, SourcePin)))
		{
			if(UEdGraphPin* InputPin = FindFirstInputPin(BreakStructNode))
			{
				if(CheckForVariableGet(CopyRecord, InputPin))
				{
					check(CopyRecord.SourcePropertyName != NAME_None);	// this should have been filled in by CheckForVariableGet() above
					CopyRecord.SourceSubStructPropertyName = *SourcePin->PinName;
					return true;
				}
			}
		}
		// could be a native break
		else if(UK2Node_CallFunction* NativeBreakNode = Cast<UK2Node_CallFunction>(FollowKnots(DestPin, SourcePin)))
		{
			UFunction* Function = NativeBreakNode->FunctionReference.ResolveMember<UFunction>(UKismetMathLibrary::StaticClass());
			if(Function && Function->HasMetaData(TEXT("NativeBreakFunc")) && IsWhitelistedNativeBreak(Function->GetFName()))
			{
				if(UEdGraphPin* InputPin = FindFirstInputPin(NativeBreakNode))
				{
					if(CheckForVariableGet(CopyRecord, InputPin))
					{
						check(CopyRecord.SourcePropertyName != NAME_None);	// this should have been filled in by CheckForVariableGet() above
						CopyRecord.SourceSubStructPropertyName = *SourcePin->PinName;
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool FUTBlueprintCompiler::FEvaluationHandlerRecord::CheckForMemberOnlyAccess(FPropertyCopyRecord& CopyRecord, UEdGraphPin* DestPin)
{
	const UAnimationGraphSchema* AnimGraphDefaultSchema = GetDefault<UAnimationGraphSchema>();

	if(DestPin)
	{
		// traverse pins to leaf nodes and check for member access/pure only
		TArray<UEdGraphPin*> PinStack;
		PinStack.Add(DestPin);
		while(PinStack.Num() > 0)
		{
			UEdGraphPin* CurrentPin = PinStack.Pop(false);
			for(auto& LinkedPin : CurrentPin->LinkedTo)
			{
				UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
				if(LinkedNode)
				{
					bool bLeafNode = true;
					for(auto& Pin : LinkedNode->Pins)
					{
						if(Pin != LinkedPin && Pin->Direction == EGPD_Input && !AnimGraphDefaultSchema->IsPosePin(Pin->PinType))
						{
							bLeafNode = false;
							PinStack.Add(Pin);
						}
					}

					if(bLeafNode)
					{
						if(UK2Node_VariableGet* LinkedVariableGetNode = Cast<UK2Node_VariableGet>(LinkedNode))
						{
							if(!LinkedVariableGetNode->IsNodePure() || !LinkedVariableGetNode->VariableReference.IsSelfContext())
							{
								// only local variable access is allowed for leaf nodes 
								CopyRecord.InvalidateFastPath();
							}
						}
						else if(UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(LinkedNode))
						{
							if(!CallFunctionNode->IsNodePure())
							{
								// only allow pure function calls
								CopyRecord.InvalidateFastPath();
							}
						}
						else if(!LinkedNode->IsA<UK2Node_TransitionRuleGetter>())
						{
							CopyRecord.InvalidateFastPath();
						}
					}
				}
			}
		}
	}

	return CopyRecord.IsFastPath();
}

void FUTBlueprintCompiler::FEvaluationHandlerRecord::ValidateFastPath(UClass* InCompiledClass)
{
	for (TPair<FName, FUTNodeSinglePropertyHandler>& ServicedPropPair : ServicedProperties)
	{
		for (FPropertyCopyRecord& CopyRecord : ServicedPropPair.Value.CopyRecords)
		{
			CopyRecord.ValidateFastPath(InCompiledClass);
		}
	}
}

void FUTBlueprintCompiler::FPropertyCopyRecord::ValidateFastPath(UClass* InCompiledClass)
{
	if (IsFastPath())
	{
		int32 DestPropertySize = DestProperty->GetSize();
		if (UArrayProperty* DestArrayProperty = Cast<UArrayProperty>(DestProperty))
		{
			DestPropertySize = DestArrayProperty->Inner->GetSize();
		}

		UProperty* SourceProperty = InCompiledClass->FindPropertyByName(SourcePropertyName);
		if (SourceProperty)
		{
			if (UArrayProperty* SourceArrayProperty = Cast<UArrayProperty>(SourceProperty))
			{
				// We dont support arrays as source properties
				InvalidateFastPath();
				return;
			}

			int32 SourcePropertySize = SourceProperty->GetSize();
			if (SourceSubStructPropertyName != NAME_None)
			{
				UProperty* SourceSubStructProperty = CastChecked<UStructProperty>(SourceProperty)->Struct->FindPropertyByName(SourceSubStructPropertyName);
				if (SourceSubStructProperty)
				{
					SourcePropertySize = SourceSubStructProperty->GetSize();
				}
				else
				{
					InvalidateFastPath();
					return;
				}
			}

			if (SourcePropertySize != DestPropertySize)
			{
				InvalidateFastPath();
				return;
			}
		}
		else
		{
			InvalidateFastPath();
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
