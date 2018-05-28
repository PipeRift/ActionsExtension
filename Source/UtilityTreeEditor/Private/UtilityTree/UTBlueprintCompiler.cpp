// Copyright 2015-2018 Piperift. All Rights Reserved.

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

#include "UtilityTreeBlueprint.h"
#include "UtilityTree/UtilityTreeGraphSchema.h"
#include "UtilityTree/AIGraphNode_Root.h"

//#include "UtilityTreeBlueprintPostCompileValidation.h" 

#define LOCTEXT_NAMESPACE "UTBlueprintCompiler"

//////////////////////////////////////////////////////////////////////////
// FUTBlueprintCompiler::FEffectiveConstantRecord

bool FUTBlueprintCompiler::FEffectiveConstantRecord::Apply(UObject* Object)
{
	uint8* StructPtr = nullptr;
	uint8* PropertyPtr = nullptr;
	
	/*if(NodeVariableProperty->Struct == FUTNode_SubInstance::StaticStruct())
	{
		PropertyPtr = ConstantProperty->ContainerPtrToValuePtr<uint8>(Object);
	}
	else*/
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
	// Determine if there is an utility tree blueprint in the ancestry of this class
	bIsDerivedUtilityTreeBlueprint = UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(UtilityTreeBlueprint) != NULL;
}

FUTBlueprintCompiler::~FUTBlueprintCompiler()
{
}

void FUTBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	FKismetCompilerContext::CreateClassVariablesFromBlueprint();
}


UEdGraphSchema_K2* FUTBlueprintCompiler::CreateSchema()
{
	UTSchema = NewObject<UUtilityTreeGraphSchema>();
	return UTSchema;
}

UK2Node_CallFunction* FUTBlueprintCompiler::SpawnCallUtilityTreeFunction(UEdGraphNode* SourceNode, FName FunctionName)
{
	//@TODO: SKELETON: This is a call on a parent function (UAnimInstance::StaticClass() specifically), should we treat it as self or not?
	UK2Node_CallFunction* FunctionCall = SpawnIntermediateNode<UK2Node_CallFunction>(SourceNode);
	FunctionCall->FunctionReference.SetSelfMember(FunctionName);
	FunctionCall->AllocateDefaultPins();

	return FunctionCall;
}

void FUTBlueprintCompiler::CreateEvaluationHandlerStruct(UAIGraphNode_Base* VisualUTNode, FEvaluationHandlerRecord& Record)
{
	// Shouldn't create a handler if there is nothing to work with
	check(Record.ServicedProperties.Num() > 0);
	check(Record.NodeVariableProperty != NULL);

	// Use the node GUID for a stable name across compiles
	FString FunctionName = FString::Printf(TEXT("%s_%s_%s_%s"), *Record.EvaluationHandlerProperty->GetName(), *VisualUTNode->GetOuter()->GetName(), *VisualUTNode->GetClass()->GetName(), *VisualUTNode->NodeGuid.ToString());
	Record.HandlerFunctionName = FName(*FunctionName);

	// check function name isn't already used (data exists that can contain duplicate GUIDs) and apply a numeric extension until it is unique
	int32 ExtensionIndex = 0;
	FName* ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
	while(ExistingName != nullptr)
	{
		FunctionName = FString::Printf(TEXT("%s_%s_%s_%s_%d"), *Record.EvaluationHandlerProperty->GetName(), *VisualUTNode->GetOuter()->GetName(), *VisualUTNode->GetClass()->GetName(), *VisualUTNode->NodeGuid.ToString(), ExtensionIndex);
		Record.HandlerFunctionName = FName(*FunctionName);
		ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
		ExtensionIndex++;
	}

	HandlerFunctionNames.Add(Record.HandlerFunctionName);
	
	// Add a custom event in the graph
	UK2Node_CustomEvent* EntryNode = SpawnIntermediateEventNode<UK2Node_CustomEvent>(VisualUTNode, nullptr, ConsolidatedEventGraph);
	EntryNode->bInternalEvent = true;
	EntryNode->CustomFunctionName = Record.HandlerFunctionName;
	EntryNode->AllocateDefaultPins();

	// The ExecChain is the current exec output pin in the linear chain
	UEdGraphPin* ExecChain = UTSchema->FindExecutionPin(*EntryNode, EGPD_Output);

	// Create a struct member write node to store the parameters into the animation node
	UK2Node_StructMemberSet* AssignmentNode = SpawnIntermediateNode<UK2Node_StructMemberSet>(VisualUTNode, ConsolidatedEventGraph);
	AssignmentNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
	AssignmentNode->StructType = Record.NodeVariableProperty->Struct;
	AssignmentNode->AllocateDefaultPins();

	// Wire up the variable node execution wires
	UEdGraphPin* ExecVariablesIn = UTSchema->FindExecutionPin(*AssignmentNode, EGPD_Input);
	ExecChain->MakeLinkTo(ExecVariablesIn);
	ExecChain = UTSchema->FindExecutionPin(*AssignmentNode, EGPD_Output);

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
				UK2Node_StructMemberGet* FetchArrayNode = SpawnIntermediateNode<UK2Node_StructMemberGet>(VisualUTNode, ConsolidatedEventGraph);
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
						UK2Node_CallArrayFunction* ArrayNode = SpawnIntermediateNode<UK2Node_CallArrayFunction>(VisualUTNode, ConsolidatedEventGraph);
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

void FUTBlueprintCompiler::CreateEvaluationHandlerInstance(UAIGraphNode_Base* VisualUTNode, FEvaluationHandlerRecord& Record)
{
	// Shouldn't create a handler if there is nothing to work with
	check(Record.ServicedProperties.Num() > 0);
	check(Record.NodeVariableProperty != nullptr);
	check(Record.bServicesInstanceProperties);

	// Use the node GUID for a stable name across compiles
	FString FunctionName = FString::Printf(TEXT("%s_%s_%s_%s"), *Record.EvaluationHandlerProperty->GetName(), *VisualUTNode->GetOuter()->GetName(), *VisualUTNode->GetClass()->GetName(), *VisualUTNode->NodeGuid.ToString());
	Record.HandlerFunctionName = FName(*FunctionName);

	// check function name isnt already used (data exists that can contain duplicate GUIDs) and apply a numeric extension until it is unique
	int32 ExtensionIndex = 0;
	FName* ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
	while(ExistingName != nullptr)
	{
		FunctionName = FString::Printf(TEXT("%s_%s_%s_%s_%d"), *Record.EvaluationHandlerProperty->GetName(), *VisualUTNode->GetOuter()->GetName(), *VisualUTNode->GetClass()->GetName(), *VisualUTNode->NodeGuid.ToString(), ExtensionIndex);
		Record.HandlerFunctionName = FName(*FunctionName);
		ExistingName = HandlerFunctionNames.Find(Record.HandlerFunctionName);
		ExtensionIndex++;
	}

	HandlerFunctionNames.Add(Record.HandlerFunctionName);

	// Add a custom event in the graph
	UK2Node_CustomEvent* EntryNode = SpawnIntermediateNode<UK2Node_CustomEvent>(VisualUTNode, ConsolidatedEventGraph);
	EntryNode->bInternalEvent = true;
	EntryNode->CustomFunctionName = Record.HandlerFunctionName;
	EntryNode->AllocateDefaultPins();

	// The ExecChain is the current exec output pin in the linear chain
	UEdGraphPin* ExecChain = UTSchema->FindExecutionPin(*EntryNode, EGPD_Output);

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
			UK2Node_VariableSet* VarAssignNode = SpawnIntermediateNode<UK2Node_VariableSet>(VisualUTNode, ConsolidatedEventGraph);
			VarAssignNode->VariableReference.SetSelfMember(CopyRecord.DestProperty->GetFName());
			VarAssignNode->AllocateDefaultPins();

			// Wire up the exec line, and update the end of the chain
			UEdGraphPin* ExecVariablesIn = UTSchema->FindExecutionPin(*VarAssignNode, EGPD_Input);
			ExecChain->MakeLinkTo(ExecVariablesIn);
			ExecChain = UTSchema->FindExecutionPin(*VarAssignNode, EGPD_Output);

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

void FUTBlueprintCompiler::ProcessUtilityTreeNode(UAIGraphNode_Base* VisualUTNode)
{
	// Early out if this node has already been processed
	if (AllocatedUTNodes.Contains(VisualUTNode))
	{
		return;
	}

	// Make sure the visual node has a runtime node template
	const UScriptStruct* NodeType = VisualUTNode->GetFNodeType();
	if (NodeType == NULL)
	{
		MessageLog.Error(TEXT("@@ has no utility tree node member"), VisualUTNode);
		return;
	}

	// Give the visual node a chance to do validation
	{
		const int32 PreValidationErrorCount = MessageLog.NumErrors;
		//VisualUTNode->ValidateUTNodeDuringCompilation(MessageLog);
		VisualUTNode->BakeDataDuringCompilation(MessageLog);
		if (MessageLog.NumErrors != PreValidationErrorCount)
		{
			return;
		}
	}

	// Create a property for the node
	const FString NodeVariableName = ClassScopeNetNameMap.MakeValidName(VisualUTNode);

	const UUtilityTreeGraphSchema* UTGraphDefaultSchema = GetDefault<UUtilityTreeGraphSchema>();

	FEdGraphPinType NodeVariableType;
	NodeVariableType.PinCategory = UTGraphDefaultSchema->PC_Struct;
	NodeVariableType.PinSubCategoryObject = NodeType;

	UStructProperty* NewProperty = Cast<UStructProperty>(CreateVariable(FName(*NodeVariableName), NodeVariableType));

	if (NewProperty == NULL)
	{
		MessageLog.Error(TEXT("Failed to create node property for @@"), VisualUTNode);
	}

	// Register this node with the compile-time data structures
	const int32 AllocatedIndex = AllocateNodeIndexCounter++;
	AllocatedUTNodes.Add(VisualUTNode, NewProperty);
	AllocatedNodePropertiesToNodes.Add(NewProperty, VisualUTNode);
	AllocatedUTNodeIndices.Add(VisualUTNode, AllocatedIndex);
	AllocatedPropertiesByIndex.Add(AllocatedIndex, NewProperty);

	UAIGraphNode_Base* TrueSourceObject = MessageLog.FindSourceObjectTypeChecked<UAIGraphNode_Base>(VisualUTNode);
	SourceNodeToProcessedNodeMap.Add(TrueSourceObject, VisualUTNode);

	// Register the slightly more permanent debug information
	NewUTBlueprintClass->GetUTBlueprintDebugData().NodePropertyToIndexMap.Add(TrueSourceObject, AllocatedIndex);
	NewUTBlueprintClass->GetUTBlueprintDebugData().NodeGuidToIndexMap.Add(TrueSourceObject->NodeGuid, AllocatedIndex);
	NewUTBlueprintClass->GetDebugData().RegisterClassPropertyAssociation(TrueSourceObject, NewProperty);

	// Node-specific compilation that requires compiler state info
	/*if (UUTGraphNode_StateMachineBase* StateMachineInstance = Cast<UUTGraphNode_StateMachineBase>(VisualUTNode))
	{
		// Compile the state machine
		ProcessStateMachine(StateMachineInstance);
	}*/

	// Record pose pins for later patch up and gather pins that have an associated evaluation handler
	TMap<FName, FEvaluationHandlerRecord> StructEvalHandlers;

	for (auto SourcePinIt = VisualUTNode->Pins.CreateIterator(); SourcePinIt; ++SourcePinIt)
	{
		UEdGraphPin* SourcePin = *SourcePinIt;
		bool bConsumed = false;

		// Register pose links for future use
		if ((SourcePin->Direction == EGPD_Input) && (UTGraphDefaultSchema->IsAIPin(SourcePin->PinType)))
		{
			// Input pose pin, going to need to be linked up
			FAILinkMappingRecord LinkRecord = VisualUTNode->GetLinkIDLocation(NodeType, SourcePin);
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

			VisualUTNode->GetPinAssociatedProperty(NodeType, SourcePin, /*out*/ SourcePinProperty, /*out*/ SourceArrayIndex);

			
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
					FString EvaluationHandlerStr = SourcePinProperty->GetMetaData(UTGraphDefaultSchema->NAME_OnEvaluate);
					FName EvaluationHandlerName(*EvaluationHandlerStr);
					if (EvaluationHandlerName == NAME_None)
					{
						EvaluationHandlerName = UTGraphDefaultSchema->DefaultEvaluationHandlerName;
					}

					FEvaluationHandlerRecord& EvalHandler = StructEvalHandlers.FindOrAdd(EvaluationHandlerName);

					EvalHandler.RegisterPin(SourcePin, SourcePinProperty, SourceArrayIndex);

					bConsumed = true;
				}

				UEdGraphPin* TrueSourcePin = MessageLog.FindSourcePin(SourcePin);
				if (TrueSourcePin)
				{
					NewUTBlueprintClass->GetDebugData().RegisterClassPropertyAssociation(TrueSourcePin, SourcePinProperty);
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
			if (StructProp->Struct == FAIExposedValueHandler::StaticStruct())
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
				CreateEvaluationHandlerInstance(VisualUTNode, Record);
			}
			else
			{
				CreateEvaluationHandlerStruct(VisualUTNode, Record);
			}

			ValidEvaluationHandlerList.Add(Record);
		}
		else
		{
			MessageLog.Error(*FString::Printf(TEXT("A property on @@ references a non-existent %s property named %s"), *(UTGraphDefaultSchema->NAME_OnEvaluate.ToString()), *(EvaluationHandlerName.ToString())), VisualUTNode);
		}
	}
}

int32 FUTBlueprintCompiler::GetAllocationIndexOfNode(UAIGraphNode_Base* VisualUTNode)
{
	ProcessUtilityTreeNode(VisualUTNode);
	int32* pResult = AllocatedUTNodeIndices.Find(VisualUTNode);
	return (pResult != NULL) ? *pResult : INDEX_NONE;
}

void FUTBlueprintCompiler::PruneIsolatedUtilityTreeNodes(const TArray<UAIGraphNode_Base*>& RootSet, TArray<UAIGraphNode_Base*>& GraphNodes)
{
	struct FNodeVisitorDownPoseWires
	{
		TSet<UEdGraphNode*> VisitedNodes;
		const UUtilityTreeGraphSchema* UTSchema;

		FNodeVisitorDownPoseWires()
		{
			UTSchema = GetDefault<UUtilityTreeGraphSchema>();
		}

		void TraverseNodes(UEdGraphNode* Node)
		{
			VisitedNodes.Add(Node);

			// Follow every exec output pin
			for (int32 i = 0; i < Node->Pins.Num(); ++i)
			{
				UEdGraphPin* MyPin = Node->Pins[i];

				if ((MyPin->Direction == EGPD_Input) && (UTSchema->IsAIPin(MyPin->PinType)))
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
		UAIGraphNode_Base* RootNode = *RootIt;
		Visitor.TraverseNodes(RootNode);
	}

	for (int32 NodeIndex = 0; NodeIndex < GraphNodes.Num(); ++NodeIndex)
	{
		UAIGraphNode_Base* Node = GraphNodes[NodeIndex];
		if (!Visitor.VisitedNodes.Contains(Node) && !IsNodePure(Node))
		{
			Node->BreakAllNodeLinks();
			GraphNodes.RemoveAtSwap(NodeIndex);
			--NodeIndex;
		}
	}
}

void FUTBlueprintCompiler::ProcessUTNodesGivenRoot(TArray<UAIGraphNode_Base*>& UTNodeList, const TArray<UAIGraphNode_Base*>& RootSet)
{
	// Now prune based on the root set
	if (MessageLog.NumErrors == 0)
	{
		PruneIsolatedUtilityTreeNodes(RootSet, UTNodeList);
	}

	// Process the remaining nodes
	for (auto SourceNodeIt = UTNodeList.CreateIterator(); SourceNodeIt; ++SourceNodeIt)
	{
		UAIGraphNode_Base* VisualUTNode = *SourceNodeIt;
		ProcessUtilityTreeNode(VisualUTNode);
	}
}

void FUTBlueprintCompiler::GetLinkedUTNodes(UAIGraphNode_Base* InGraphNode, TArray<UAIGraphNode_Base*> &LinkedUTNodes)
{
	for(UEdGraphPin* Pin : InGraphNode->Pins)
	{
		if(Pin->Direction == EEdGraphPinDirection::EGPD_Input &&
		   Pin->PinType.PinCategory == TEXT("struct"))
		{
			if(UScriptStruct* Struct = Cast<UScriptStruct>(Pin->PinType.PinSubCategoryObject.Get()))
			{
				if(Struct->IsChildOf(FAILinkBase::StaticStruct()))
				{
					GetLinkedUTNodes_TraversePin(Pin, LinkedUTNodes);
				}
			}
		}
	}
}

void FUTBlueprintCompiler::GetLinkedUTNodes_TraversePin(UEdGraphPin* InPin, TArray<UAIGraphNode_Base*>& LinkedUTNodes)
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
		else if(UAIGraphNode_Base* UTNode = Cast<UAIGraphNode_Base>(OwningNode))
		{
			GetLinkedUTNodes_ProcessUTNode(UTNode, LinkedUTNodes);
		}
	}
}

void FUTBlueprintCompiler::GetLinkedUTNodes_ProcessUTNode(UAIGraphNode_Base* UTNode, TArray<UAIGraphNode_Base *> &LinkedUTNodes)
{
	if(!AllocatedUTNodes.Contains(UTNode))
	{
		UAIGraphNode_Base* TrueSourceNode = MessageLog.FindSourceObjectTypeChecked<UAIGraphNode_Base>(UTNode);

		if(UAIGraphNode_Base** AllocatedNode = SourceNodeToProcessedNodeMap.Find(TrueSourceNode))
		{
			LinkedUTNodes.Add(*AllocatedNode);
		}
		else
		{
			FString ErrorString = FString::Printf(*LOCTEXT("MissingLink", "Missing allocated node for %s while searching for node links - likely due to the node having outstanding errors.").ToString(), *UTNode->GetName());
			MessageLog.Error(*ErrorString);
		}
	}
	else
	{
		LinkedUTNodes.Add(UTNode);
	}
}

void FUTBlueprintCompiler::ProcessAllUtilityTreeNodes()
{
	// Validate the graph
	ValidateGraphIsWellFormed(ConsolidatedEventGraph);


	// Build the raw node list
	TArray<UAIGraphNode_Base*> UTNodeList;
	ConsolidatedEventGraph->GetNodesOfClass<UAIGraphNode_Base>(/*out*/ UTNodeList);

	//TArray<UK2Node_TransitionRuleGetter*> Getters;
	//ConsolidatedEventGraph->GetNodesOfClass<UK2Node_TransitionRuleGetter>(/*out*/ Getters);

	// Get utility tree getters from the root graph (processing the nodes below will collect them in nested graphs)
	//TArray<UK2Node_UTGetter*> RootGraphAnimGetters;
	//ConsolidatedEventGraph->GetNodesOfClass<UK2Node_UTGetter>(RootGraphAnimGetters);

	// Find the root node
	UAIGraphNode_Root* PrePhysicsRoot = NULL;
	TArray<UAIGraphNode_Base*> RootSet;

	AllocateNodeIndexCounter = 0;
	NewUTBlueprintClass->RootUTNodeIndex = 0;//INDEX_NONE;

	for (auto SourceNodeIt = UTNodeList.CreateIterator(); SourceNodeIt; ++SourceNodeIt)
	{
		UAIGraphNode_Base* SourceNode = *SourceNodeIt;
		UAIGraphNode_Base* TrueNode = MessageLog.FindSourceObjectTypeChecked<UAIGraphNode_Base>(SourceNode);
		TrueNode->BlueprintUsage = EUTBlueprintUsage::NoProperties;

		if (UAIGraphNode_Root* PossibleRoot = Cast<UAIGraphNode_Root>(SourceNode))
		{
			if (UAIGraphNode_Root* Root = ExactCast<UAIGraphNode_Root>(PossibleRoot))
			{
				if (PrePhysicsRoot != NULL)
				{
					MessageLog.Error(*FString::Printf(*LOCTEXT("ExpectedOneFunctionEntry_Error", "Expected only one AI root, but found both @@ and @@").ToString()),
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
		/*for (auto GetterIt = Getters.CreateIterator(); GetterIt; ++GetterIt)
		{
			ProcessTransitionGetter(*GetterIt, NULL); // transition nodes should not appear at top-level
		}*/

		// Wire root getters
		/*for(UK2Node_UTGetter* RootGraphGetter : RootGraphAnimGetters)
		{
			AutoWireUTGetter(RootGraphGetter, nullptr);
		}*/

		// Wire nested getters
		/*for(UK2Node_UTGetter* Getter : FoundGetterNodes)
		{
			AutoWireUTGetter(Getter, nullptr);
		}*/

		NewUTBlueprintClass->RootUTNodeIndex = GetAllocationIndexOfNode(PrePhysicsRoot);
	}
	else
	{
		MessageLog.Error(*FString::Printf(*LOCTEXT("ExpectedAFunctionEntry_Error", "Expected an animation root, but did not find one").ToString()));
	}
}

void FUTBlueprintCompiler::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	Super::CopyTermDefaultsToDefaultObject(DefaultObject);

	if (bIsDerivedUtilityTreeBlueprint)
	{
		// If we are a derived animation graph; apply any stored overrides.
		// Restore values from the root class to catch values where the override has been removed.
		UUTBlueprintGeneratedClass* RootUTClass = NewUTBlueprintClass;
		while(UUTBlueprintGeneratedClass* NextClass = Cast<UUTBlueprintGeneratedClass>(RootUTClass->GetSuperClass()))
		{
			RootUTClass = NextClass;
		}
		UObject* RootDefaultObject = RootUTClass->GetDefaultObject();

		for (TFieldIterator<UProperty> It(RootUTClass) ; It; ++It)
		{
			UProperty* RootProp = *It;

			if (UStructProperty* RootStructProp = Cast<UStructProperty>(RootProp))
			{
				if (RootStructProp->Struct->IsChildOf(FAINode_Base::StaticStruct()))
				{
					UStructProperty* ChildStructProp = FindField<UStructProperty>(NewUTBlueprintClass, *RootStructProp->GetName());
					check(ChildStructProp);
					uint8* SourcePtr = RootStructProp->ContainerPtrToValuePtr<uint8>(RootDefaultObject);
					uint8* DestPtr = ChildStructProp->ContainerPtrToValuePtr<uint8>(DefaultObject);
					check(SourcePtr && DestPtr);
					RootStructProp->CopyCompleteValue(DestPtr, SourcePtr);
				}
			}
		}

		// Patch the overridden values into the CDO

		TArray<FUTParentNodeAssetOverride*> AssetOverrides;
		UtilityTreeBlueprint->GetAssetOverrides(AssetOverrides);
		for (FUTParentNodeAssetOverride* Override : AssetOverrides)
		{
			if (Override->NewAsset)
			{
				FAINode_Base* BaseNode = NewUTBlueprintClass->GetPropertyInstance<FAINode_Base>(DefaultObject, Override->ParentNodeGuid, EAIPropertySearchMode::Hierarchy);
				if (BaseNode)
				{
					//BaseNode->OverrideAsset(Override->NewAsset);
				}
			}
		}

		return;
	}

	int32 LinkIndexCount = 0;
	TMap<UAIGraphNode_Base*, int32> LinkIndexMap;
	TMap<UAIGraphNode_Base*, uint8*> NodeBaseAddresses;

	// Initialize animation nodes from their templates
	for (TFieldIterator<UProperty> It(DefaultObject->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UProperty* TargetProperty = *It;

		if (UAIGraphNode_Base* VisualUTNode = AllocatedNodePropertiesToNodes.FindRef(TargetProperty))
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
		FAILinkMappingRecord& Record = *PoseLinkIt;

		UAIGraphNode_Base* LinkingNode = Record.GetLinkingNode();
		UAIGraphNode_Base* LinkedNode = Record.GetLinkedNode();
		
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
	for (auto LiteralLinkIt = ValidUTNodePinConstants.CreateIterator(); LiteralLinkIt; ++LiteralLinkIt)
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
		ProcessAllUtilityTreeNodes();
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
	check(ClassToClean == NewClass && NewUTBlueprintClass == NewClass);

	NewUTBlueprintClass->UTBlueprintDebugData = FUTBlueprintDebugData();

	// Reset the baked data
	//@TODO: Move this into PurgeClass

	NewUTBlueprintClass->RootUTNodeIndex = INDEX_NONE;
	NewUTBlueprintClass->RootUTNodeProperty = NULL;
	NewUTBlueprintClass->OrderedSavedPoseIndices.Empty();
	NewUTBlueprintClass->UTNodeProperties.Empty();

	// Copy over runtime data from the blueprint to the class


	UUtilityTreeBlueprint* RootUTBP = UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(UtilityTreeBlueprint);
	bIsDerivedUtilityTreeBlueprint = RootUTBP != NULL;

	// Copy up data from a parent utilitytree blueprint
	if (bIsDerivedUtilityTreeBlueprint)
	{
		UUTBlueprintGeneratedClass* RootUTClass = CastChecked<UUTBlueprintGeneratedClass>(RootUTBP->GeneratedClass);

		NewUTBlueprintClass->RootUTNodeIndex = RootUTClass->RootUTNodeIndex;
		NewUTBlueprintClass->OrderedSavedPoseIndices = RootUTClass->OrderedSavedPoseIndices;
	}
}

void FUTBlueprintCompiler::PostCompile()
{
	Super::PostCompile();

	UUTBlueprintGeneratedClass* UTBlueprintGeneratedClass = CastChecked<UUTBlueprintGeneratedClass>(NewClass);

	UUtilityTree* DefaultUtilityTree = CastChecked<UUtilityTree>(UTBlueprintGeneratedClass->GetDefaultObject());


	for (const FEffectiveConstantRecord& ConstantRecord : ValidUTNodePinConstants)
	{
		UAIGraphNode_Base* Node = CastChecked<UAIGraphNode_Base>(ConstantRecord.LiteralSourcePin->GetOwningNode());
		UAIGraphNode_Base* TrueNode = MessageLog.FindSourceObjectTypeChecked<UAIGraphNode_Base>(Node);
		TrueNode->BlueprintUsage = EUTBlueprintUsage::DoesNotUseBlueprint;
	}

	for(const FEvaluationHandlerRecord& EvaluationHandler : ValidEvaluationHandlerList)
	{
		if(EvaluationHandler.ServicedProperties.Num() > 0)
		{
			const FUTNodeSinglePropertyHandler& Handler = EvaluationHandler.ServicedProperties.CreateConstIterator()->Value;
			check(Handler.CopyRecords.Num() > 0);
			check(Handler.CopyRecords[0].DestPin != nullptr);
			UAIGraphNode_Base* Node = CastChecked<UAIGraphNode_Base>(Handler.CopyRecords[0].DestPin->GetOwningNode());
			UAIGraphNode_Base* TrueNode = MessageLog.FindSourceObjectTypeChecked<UAIGraphNode_Base>(Node);	

			FAIExposedValueHandler* HandlerPtr = EvaluationHandler.EvaluationHandlerProperty->ContainerPtrToValuePtr<FAIExposedValueHandler>(EvaluationHandler.NodeVariableProperty->ContainerPtrToValuePtr<void>(DefaultUtilityTree));
			TrueNode->BlueprintUsage = HandlerPtr->BoundFunction != NAME_None ? EUTBlueprintUsage::UsesBlueprint : EUTBlueprintUsage::DoesNotUseBlueprint;
		}
	}

	/*for (UPoseWatch* PoseWatch : UtilityTreeBlueprint->PoseWatches)
	{
		//UtilityTreeEditorUtils::SetPoseWatch(PoseWatch, UtilityTreeBlueprint);
	}*/

	// iterate all ut node and call PostCompile
	for (UStructProperty* Property : TFieldRange<UStructProperty>(UTBlueprintGeneratedClass, EFieldIteratorFlags::IncludeSuper))
	{
		/*if (Property->Struct->IsChildOf(FUTNode_Base::StaticStruct()))
		{
			FUTNode_Base* UTNode = Property->ContainerPtrToValuePtr<FUTNode_Base>(DefaultUTInstance);
			UTNode->PostCompile();
		}*/
	}
}

void FUTBlueprintCompiler::CreateFunctionList()
{
	// (These will now be processed after uber graph merge)

	// Build the list of functions and do preprocessing on all of them
	Super::CreateFunctionList();
}

void FUTBlueprintCompiler::DumpUTDebugData()
{
	// List all compiled-down nodes and their sources
	if (NewUTBlueprintClass->RootUTNodeProperty == NULL)
	{
		return;
	}

	int32 RootIndex = INDEX_NONE;
	NewUTBlueprintClass->UTNodeProperties.Find(NewUTBlueprintClass->RootUTNodeProperty, /*out*/ RootIndex);
	
	uint8* CDOBase = (uint8*)NewUTBlueprintClass->ClassDefaultObject;

	MessageLog.Note(*FString::Printf(TEXT("Anim Root is #%d"), RootIndex));
	for (int32 Index = 0; Index < NewUTBlueprintClass->UTNodeProperties.Num(); ++Index)
	{
		UStructProperty* NodeProperty = NewUTBlueprintClass->UTNodeProperties[Index];
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
				if (ChildStructProp->Struct->IsChildOf(FAILinkBase::StaticStruct()))
				{
					uint8* ChildPropertyPtr =  ChildStructProp->ContainerPtrToValuePtr<uint8>(NodeProperty->ContainerPtrToValuePtr<uint8>(CDOBase));
					FAILinkBase* ChildPoseLink = (FAILinkBase*)ChildPropertyPtr;

					if (ChildPoseLink->LinkID != INDEX_NONE)
					{
						UStructProperty* LinkedProperty = NewUTBlueprintClass->UTNodeProperties[ChildPoseLink->LinkID];
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
}

void FUTBlueprintCompiler::FEvaluationHandlerRecord::PatchFunctionNameAndCopyRecordsInto(UObject* TargetObject) const
{
	FAIExposedValueHandler* HandlerPtr = EvaluationHandlerProperty->ContainerPtrToValuePtr<FAIExposedValueHandler>(NodeVariableProperty->ContainerPtrToValuePtr<void>(TargetObject));
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

				  FAIExposedValueCopyRecord CopyRecord;
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
	const UUtilityTreeGraphSchema* UTSchema = GetDefault<UUtilityTreeGraphSchema>();

	for(UEdGraphPin* Pin : InNode->Pins)
	{
		if(Pin && Pin->Direction == EGPD_Input && !UTSchema->IsExecPin(*Pin) && !UTSchema->IsSelfPin(*Pin))
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
	// Optimize UtilityTree Blueprint Member Variable Access

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
					CopyRecord.Operation = EAIPostCopyOperation::LogicalNegateBool;
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
	const UUtilityTreeGraphSchema* UTGraphDefaultSchema = GetDefault<UUtilityTreeGraphSchema>();

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
						if(Pin != LinkedPin && Pin->Direction == EGPD_Input && !UTGraphDefaultSchema->IsAIPin(Pin->PinType))
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
						/*else if(!LinkedNode->IsA<UK2Node_TransitionRuleGetter>())
						{
							CopyRecord.InvalidateFastPath();
						}*/
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
