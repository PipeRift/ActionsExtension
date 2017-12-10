// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "UtilityTree/UtilityTreeGraphSchema.h"

#include "K2Node.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Knot.h"
#include "ScopedTransaction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

//#include "UtilityTree/AnimationAsset.h"
#include "UtilityTreeBlueprint.h"
//#include "UtilityTree/UTNodeBase.h"
//#include "UtilityTreeGraphCommands.h"

#define LOCTEXT_NAMESPACE "UtilityTreeGraphSchema"


/////////////////////////////////////////////////////
// UUtilityTreeGraphSchema

UUtilityTreeGraphSchema::UUtilityTreeGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PN_SequenceName = TEXT("Sequence");

	NAME_NeverAsPin = TEXT("NeverAsPin");
	NAME_PinHiddenByDefault = TEXT("PinHiddenByDefault");
	NAME_PinShownByDefault = TEXT("PinShownByDefault");
	NAME_AlwaysAsPin = TEXT("AlwaysAsPin");
	NAME_OnEvaluate = TEXT("OnEvaluate");
	NAME_CustomizeProperty = TEXT("CustomizeProperty");
	DefaultEvaluationHandlerName = TEXT("EvaluateGraphExposedInputs");
}

FLinearColor UUtilityTreeGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	const bool bAdditive = PinType.PinSubCategory == TEXT("Additive");
	if (UUtilityTreeGraphSchema::IsPosePin(PinType))
	{
		if (bAdditive) 
		{
			return FLinearColor(0.12, 0.60, 0.10);
		}
		else
		{
			return FLinearColor::White;
		}
	}

	return Super::GetPinTypeColor(PinType);
}

EGraphType UUtilityTreeGraphSchema::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_Ubergraph;
}

void UUtilityTreeGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	// Create the result node
	FGraphNodeCreator<UUTGraphNode_Root> NodeCreator(Graph);
	UUTGraphNode_Root* ResultSinkNode = NodeCreator.CreateNode();
	NodeCreator.Finalize();
	SetNodeMetaData(ResultSinkNode, FNodeMetadata::DefaultGraphNode);
}

void UUtilityTreeGraphSchema::HandleGraphBeingDeleted(UEdGraph& GraphBeingRemoved) const
{
	if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(&GraphBeingRemoved))
	{
	}
}

void UUtilityTreeGraphSchema::GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const
{
	DisplayInfo.DocLink = TEXT("Shared/Editors/BlueprintEditor/GraphTypes");
	DisplayInfo.PlainName = FText::FromString(Graph.GetName()); // Fallback is graph name


	DisplayInfo.PlainName = LOCTEXT("GraphDisplayName_UtilityTreeGraph", "UtilityGraph");

	DisplayInfo.Tooltip = LOCTEXT("GraphTooltip_UtilityTreeGraph", "Graph used to blend together different behaviors.");
	DisplayInfo.DocExcerptName = TEXT("UtilityTreeGraph");
}

bool UUtilityTreeGraphSchema::IsPosePin(const FEdGraphPinType& PinType)
{
	const UUtilityTreeGraphSchema* Schema = GetDefault<UUtilityTreeGraphSchema>();

	//const UScriptStruct* PoseLinkStruct = FPoseLink::StaticStruct();
	return false;// (PinType.PinCategory == Schema->PC_Struct) && (PinType.PinSubCategoryObject == PoseLinkStruct);
}

bool UUtilityTreeGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	UEdGraphPin* OutputPin = nullptr;
	UEdGraphPin* InputPin = nullptr;

	if(A->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		OutputPin = A;
		InputPin = B;
	}
	else
	{
		OutputPin = B;
		InputPin = A;
	}
	check(OutputPin && InputPin);

	UEdGraphNode* OutputNode = OutputPin->GetOwningNode();

	if(UK2Node_Knot* RerouteNode = Cast<UK2Node_Knot>(OutputNode))
	{
		// Double check this is our "exec"-like line
		bool bOutputIsPose = IsPosePin(OutputPin->PinType);
		bool bInputIsPose = IsPosePin(InputPin->PinType);
		bool bHavePosePin = bOutputIsPose || bInputIsPose;
		bool bHaveWildPin = InputPin->PinType.PinCategory == PC_Wildcard || OutputPin->PinType.PinCategory == PC_Wildcard;

		if((bOutputIsPose && bInputIsPose) || (bHavePosePin && bHaveWildPin))
		{
			// Ok this is a valid exec-like line, we need to kill any connections already on the output pin
			OutputPin->BreakAllPinLinks();
		}
	}

	return Super::TryCreateConnection(A, B);
}

const FPinConnectionResponse UUtilityTreeGraphSchema::DetermineConnectionResponseOfCompatibleTypedPins(const UEdGraphPin* PinA, const UEdGraphPin* PinB, const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	// Enforce a tree hierarchy; where poses can only have one output (parent) connection
	if (IsPosePin(OutputPin->PinType) && IsPosePin(InputPin->PinType))
	{
		if ((OutputPin->LinkedTo.Num() > 0) || (InputPin->LinkedTo.Num() > 0))
		{
			const ECanCreateConnectionResponse ReplyBreakOutputs = CONNECT_RESPONSE_BREAK_OTHERS_AB;
			return FPinConnectionResponse(ReplyBreakOutputs, TEXT("Replace existing connections"));
		}
	}

	// Fall back to standard K2 rules
	return Super::DetermineConnectionResponseOfCompatibleTypedPins(PinA, PinB, InputPin, OutputPin);
}

bool UUtilityTreeGraphSchema::ArePinsCompatible(const UEdGraphPin* PinA, const UEdGraphPin* PinB, const UClass* CallingContext, bool bIgnoreArray) const
{
	// both are pose pin, but doesn't match type, then return false;
	if (IsPosePin(PinA->PinType) && IsPosePin(PinB->PinType))
	{
		return false;
	}

	// Disallow pose pins connecting to wildcards (apart from reroute nodes)
	if(IsPosePin(PinA->PinType) && PinB->PinType.PinCategory == PC_Wildcard)
	{
		return Cast<UK2Node_Knot>(PinB->GetOwningNode()) != nullptr;
	}
	else if(IsPosePin(PinB->PinType) && PinA->PinType.PinCategory == PC_Wildcard)
	{
		return Cast<UK2Node_Knot>(PinA->GetOwningNode()) != nullptr;
	}

	return Super::ArePinsCompatible(PinA, PinB, CallingContext, bIgnoreArray);
}

bool UUtilityTreeGraphSchema::DoesSupportAnimNotifyActions() const
{
	// Don't offer notify items in anim graph
	return false;
}

bool UUtilityTreeGraphSchema::SearchForAutocastFunction(const UEdGraphPin* OutputPin, const UEdGraphPin* InputPin, FName& TargetFunction, /*out*/ UClass*& FunctionOwner) const
{
	return Super::SearchForAutocastFunction(OutputPin, InputPin, TargetFunction, FunctionOwner);
}

bool UUtilityTreeGraphSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	// Determine which pin is an input and which pin is an output
	UEdGraphPin* InputPin = NULL;
	UEdGraphPin* OutputPin = NULL;
	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return false;
	}

	// Give the regular conversions a shot
	return Super::CreateAutomaticConversionNodeAndConnections(PinA, PinB);
}

/*void UUtilityTreeGraphSchema::SpawnNodeFromAsset(UUtilityTreeAsset* Asset, const FVector2D& GraphPosition, UEdGraph* Graph, UEdGraphPin* PinIfAvailable)
{
	check(Graph);
	check(Graph->GetSchema()->IsA(UUtilityTreeGraphSchema::StaticClass()));
	check(Asset);
	...
}*/

/*void UUtilityTreeGraphSchema::UpdateNodeWithAsset(UK2Node* K2Node, UUtilityTreeAsset* Asset)
{
	if (Asset != NULL)
	{
	}
}*/


void UUtilityTreeGraphSchema::DroppedAssetsOnGraph( const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph ) const 
{
	/*UUtilityTreeAsset* Asset = FAssetData::GetFirstAsset<UUtilityTreeAsset>(Assets);
	if ((Asset != NULL) && (Graph != NULL))
	{
		SpawnNodeFromAsset(Asset, GraphPosition, Graph, NULL);
	}*/
}



void UUtilityTreeGraphSchema::DroppedAssetsOnNode(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphNode* Node) const
{
	/*UUtilityTreeAsset* Asset = FAssetData::GetFirstAsset<UUtilityTreeAsset>(Assets);
	UK2Node* K2Node = Cast<UK2Node>(Node);
	if ((Asset != NULL) && (K2Node!= NULL))
	{
		UpdateNodeWithAsset(K2Node, Asset);
	}*/
}

void UUtilityTreeGraphSchema::DroppedAssetsOnPin(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraphPin* Pin) const
{
	/*UUtilityTreeAsset* Asset = FAssetData::GetFirstAsset<UUtilityTreeAsset>(Assets);
	if ((Asset != NULL) && (Pin != NULL))
	{
		SpawnNodeFromAsset(Asset, GraphPosition, Pin->GetOwningNode()->GetGraph(), Pin);
	}*/
}

void UUtilityTreeGraphSchema::GetAssetsNodeHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphNode* HoverNode, FString& OutTooltipText, bool& OutOkIcon) const 
{ 
	OutTooltipText = TEXT("");
	OutOkIcon = false;
	return;
}

void UUtilityTreeGraphSchema::GetAssetsPinHoverMessage(const TArray<FAssetData>& Assets, const UEdGraphPin* HoverPin, FString& OutTooltipText, bool& OutOkIcon) const 
{
	OutTooltipText = TEXT("");
	OutOkIcon = false;
	return;
}

void UUtilityTreeGraphSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
}

void UUtilityTreeGraphSchema::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	Super::GetContextMenuActions(CurrentGraph, InGraphNode, InGraphPin, MenuBuilder, bIsDebugging);

	/*if (const UUTGraphNode_Base* UTGraphNode = Cast<const UUTGraphNode_Base>(InGraphNode))
	{
	}*/
}

FText UUtilityTreeGraphSchema::GetPinDisplayName(const UEdGraphPin* Pin) const 
{
	check(Pin != NULL);

	FText DisplayName = Super::GetPinDisplayName(Pin);

	/*if (UUTGraphNode_Base* Node = Cast<UUTGraphNode_Base>(Pin->GetOwningNode()))
	{
		FString ProcessedDisplayName = DisplayName.ToString();
		Node->PostProcessPinName(Pin, ProcessedDisplayName);
		DisplayName = FText::FromString(ProcessedDisplayName);
	}*/

	return DisplayName;
}

#undef LOCTEXT_NAMESPACE
