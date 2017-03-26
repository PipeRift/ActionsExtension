// Copyright 2015-2017 Piperift. All Rights Reserved.
 
#include "AIExtensionEditorPrivatePCH.h"
 
#include "BPBehaviourTreeComponent.h"
#include "BPBT_Node.h"
 
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "K2Node_CallFunction.h"
 
#include "K2Node_BTNode.h"
 
#define LOCTEXT_NAMESPACE "AIExtensionEditor"
 
//Helper which will store one of the function inputs we excpect BP callable function will have.
struct FK2Node_BTNodeHelper
{
	static FString OwningPlayerPinName;
};
 
FString FK2Node_BTNodeHelper::OwningPlayerPinName(TEXT("OwningPlayer"));
 
UK2Node_BTNode::UK2Node_BTNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Adds a Behaviour Tree node.");
}
 
//Adds default pins to node. These Pins (inputs ?) are always displayed.
void UK2Node_BTNode::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
 
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
 
	// OwningPlayer pin
	UEdGraphPin* OwningPlayerPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), APlayerController::StaticClass(), false, false, FK2Node_BTNodeHelper::OwningPlayerPinName);
	SetPinToolTip(*OwningPlayerPin, LOCTEXT("OwningPlayerPinDescription", "The player that 'owns' the this item."));
}
 
FLinearColor UK2Node_BTNode::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}
 
FText UK2Node_BTNode::GetBaseNodeTitle() const
{
	return LOCTEXT("BTNode_BaseTitle", "Node");
}
 
FText UK2Node_BTNode::GetNodeTitleFormat() const
{
	return LOCTEXT("BTNode", "{ClassName}");
}
 
//which class can be used with this node to create objects. All childs of class can be used.
UClass* UK2Node_BTNode::GetClassPinBaseClass() const
{
	return UBPBT_Node::StaticClass();
}
 
//Set context menu category in which our node will be present.
FText UK2Node_BTNode::GetMenuCategory() const
{
	return FText::FromString("Game Inventory System");
}
 
//gets out predefined pin
UEdGraphPin* UK2Node_BTNode::GetOwningPlayerPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_BTNodeHelper::OwningPlayerPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}
 
bool UK2Node_BTNode::IsSpawnVarPin(UEdGraphPin* Pin)
{
	return(Super::IsSpawnVarPin(Pin) &&
		Pin->PinName != FK2Node_BTNodeHelper::OwningPlayerPinName);
}
 
//and this is where magic really happens. This will expand node for our custom object, with properties
//which are set as EditAwnywhere and meta=(ExposeOnSpawn), or equivalent in blueprint.
void UK2Node_BTNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
 
	//look for static function in BlueprintFunctionLibrary
	//In this class and of this name
	static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UBPBehaviourTreeComponent, Node);
	//with these inputs (as a Side note, these should be probabaly FName not FString)
	static FString WorldContextObject_ParamName = FString(TEXT("WorldContextObject"));
	static FString WidgetType_ParamName = FString(TEXT("ItemType"));
	static FString OwningPlayer_ParamName = FString(TEXT("OwningPlayer"));
 
	//get pointer to self;
	UK2Node_BTNode* CreateItemDataNode = this;
 
	//get pointers to default pins.
	//Exec pins are those big arrows, connected with thick white lines.
	UEdGraphPin* SpawnNodeExec = CreateItemDataNode->GetExecPin();
	//gets world context pin from our static function
	UEdGraphPin* SpawnWorldContextPin = CreateItemDataNode->GetWorldContextPin();
	//the same as above
	UEdGraphPin* SpawnOwningPlayerPin = CreateItemDataNode->GetOwningPlayerPin();
	//get class pin which is used to determine which class to spawn.
	UEdGraphPin* SpawnClassPin = CreateItemDataNode->GetClassPin();
	//then pin is the same as exec pin, just on the other side (the out arrow).
	UEdGraphPin* SpawnNodeThen = CreateItemDataNode->GetThenPin();
	//result pin, which will output our spawned object.
	UEdGraphPin* SpawnNodeResult = CreateItemDataNode->GetResultPin();
 
	UClass* SpawnClass = (SpawnClassPin != NULL) ? Cast<UClass>(SpawnClassPin->DefaultObject) : NULL;
	if ((0 == SpawnClassPin->LinkedTo.Num()) && (NULL == SpawnClass))
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("CreateItemDAtaNodeMissingClass_Error", "Spawn node @@ must have a class specified.").ToString(), CreateItemDataNode);
		// we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
		CreateItemDataNode->BreakAllNodeLinks();
		return;
	}
 
	//////////////////////////////////////////////////////////////////////////
	// create 'UWidgetBlueprintLibrary::Create' call node
	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(CreateItemDataNode, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(Create_FunctionName, UK2Node_BTNode::StaticClass());
	CallCreateNode->AllocateDefaultPins();
 
	//allocate nodes for created widget.
	UEdGraphPin* CallCreateExec = CallCreateNode->GetExecPin();
	UEdGraphPin* CallCreateWorldContextPin = CallCreateNode->FindPinChecked(WorldContextObject_ParamName);
	UEdGraphPin* CallCreateWidgetTypePin = CallCreateNode->FindPinChecked(WidgetType_ParamName);
	UEdGraphPin* CallCreateOwningPlayerPin = CallCreateNode->FindPinChecked(OwningPlayer_ParamName);
	UEdGraphPin* CallCreateResult = CallCreateNode->GetReturnValuePin();
 
	// Move 'exec' connection from create widget node to 'UWidgetBlueprintLibrary::Create'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec, *CallCreateExec);
 
	if (SpawnClassPin->LinkedTo.Num() > 0)
	{
		// Copy the 'blueprint' connection from the spawn node to 'UWidgetBlueprintLibrary::Create'
		CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallCreateWidgetTypePin);
	}
	else
	{
		// Copy blueprint literal onto 'UWidgetBlueprintLibrary::Create' call 
		CallCreateWidgetTypePin->DefaultObject = SpawnClass;
	}
 
	// Copy the world context connection from the spawn node to 'UWidgetBlueprintLibrary::Create' if necessary
	if (SpawnWorldContextPin)
	{
		CompilerContext.MovePinLinksToIntermediate(*SpawnWorldContextPin, *CallCreateWorldContextPin);
	}
 
	// Copy the 'Owning Player' connection from the spawn node to 'UWidgetBlueprintLibrary::Create'
	CompilerContext.MovePinLinksToIntermediate(*SpawnOwningPlayerPin, *CallCreateOwningPlayerPin);
 
	// Move result connection from spawn node to 'UWidgetBlueprintLibrary::Create'
	CallCreateResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult, *CallCreateResult);
 
	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes
 
	// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, CreateItemDataNode, CallCreateResult, GetClassToSpawn());
 
	// Move 'then' connection from create widget node to the last 'then'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *LastThen);
 
	// Break any links to the expanded node
	CreateItemDataNode->BreakAllNodeLinks();
}
 
#undef LOCTEXT_NAMESPACE