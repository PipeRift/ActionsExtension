// Copyright 2015-2017 Piperift. All Rights Reserved.
 
#include "AIExtensionEditorPrivatePCH.h"
 
#include "TaskFunctionLibrary.h"
#include "BPBT_Node.h"
 
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "K2Node_CallFunction.h"
 
#include "K2Node_BTNode.h"
 
#define LOCTEXT_NAMESPACE "AIExtensionEditor"
 
UK2Node_BTNode::UK2Node_BTNode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeTooltip = LOCTEXT("NodeTooltip", "Creates a new task object");
}
 
//Adds default pins to node. These Pins (inputs ?) are always displayed.
void UK2Node_BTNode::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();
 
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
}
 
FLinearColor UK2Node_BTNode::GetNodeTitleColor() const
{
    return Super::GetNodeTitleColor();
}
 
FText UK2Node_BTNode::GetBaseNodeTitle() const
{
    return LOCTEXT("BTNode_BaseTitle", "Create Task");
}
 
FText UK2Node_BTNode::GetNodeTitleFormat() const
{
    return LOCTEXT("BTTask", "{ClassName}");
}
 
//which class can be used with this node to create objects. All childs of class can be used.
UClass* UK2Node_BTNode::GetClassPinBaseClass() const
{
    return UBPBT_Node::StaticClass();
}
 
//Set context menu category in which our node will be present.
FText UK2Node_BTNode::GetMenuCategory() const
{
    return FText::FromString("Behaviour Tree");
}
 
bool UK2Node_BTNode::IsSpawnVarPin(UEdGraphPin* Pin)
{
    return Super::IsSpawnVarPin(Pin);
}
 
//and this is where magic really happens. This will expand node for our custom object, with properties
//which are set as EditAwnywhere and meta=(ExposeOnSpawn), or equivalent in blueprint.
void UK2Node_BTNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
 
    //Get static function
    static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UTaskFunctionLibrary, CreateTask);

    //Set function parameter names
    static FString ParamName_WorldContextObject = FString(TEXT("WorldContextObject"));
    static FString ParamName_WidgetType = FString(TEXT("TaskType"));
 
    //Save self
    UK2Node_BTNode* TaskNode = this;
 
    /* Retrieve Pins */
    //Exec
    UEdGraphPin* ExecPin         = TaskNode->GetExecPin();         // Exec pins are those big arrows, connected with thick white lines.   
    UEdGraphPin* ThenPin         = TaskNode->GetThenPin();         // Then pin is the same as exec pin, just on the other side (the out arrow).
    //Inputs
    UEdGraphPin* WorldContextPin = TaskNode->GetWorldContextPin(); // Gets world context pin from our static function
    UEdGraphPin* ClassPin        = TaskNode->GetClassPin();        // Get class pin which is used to determine which class to spawn.
    //Outputs
    UEdGraphPin* ResultPin       = TaskNode->GetResultPin();       // Result pin, which will output our spawned object.



    UClass* SpawnClass = ClassPin ? Cast<UClass>(ClassPin->DefaultObject) : NULL;

    //Don't proceed if ClassPin is not defined or valid
    if (ClassPin->LinkedTo.Num() == 0 && NULL == SpawnClass)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "Spawn node @@ must have a class specified.").ToString(), TaskNode);
        // we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
        TaskNode->BreakAllNodeLinks();
        return;
    }


 
    //////////////////////////////////////////////////////////////////////////
    // create 'UTaskFunctionLibrary::CreateTask' call node
    UK2Node_CallFunction* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(TaskNode, SourceGraph);
    //Attach function
    CallFunctionNode->FunctionReference.SetExternalMember(Create_FunctionName, UK2Node_BTNode::StaticClass());
    CallFunctionNode->AllocateDefaultPins();
 
    //allocate nodes for created widget.
    UEdGraphPin* CallFunction_Exec = CallFunctionNode->GetExecPin();
    UEdGraphPin* CallFunction_WorldContext = CallFunctionNode->FindPinChecked(ParamName_WorldContextObject);
    UEdGraphPin* CallFunction_WidgetType = CallFunctionNode->FindPinChecked(ParamName_WidgetType);
    UEdGraphPin* CallFunction_Result = CallFunctionNode->GetReturnValuePin();
 
    // Move 'exec' pin to 'UTaskFunctionLibrary::CreateTask'
    CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallFunction_Exec);
 
    if (ClassPin->LinkedTo.Num() > 0) {
        CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CallFunction_WidgetType);
    } else {
        // Copy blueprint literal onto 'UTaskFunctionLibrary::CreateTask' call 
        CallFunction_WidgetType->DefaultObject = SpawnClass;
    }
 
    // Copy WorldContext pin to 'UTaskFunctionLibrary::CreateTask' if necessary
    if (WorldContextPin)
        CompilerContext.MovePinLinksToIntermediate(*WorldContextPin, *CallFunction_WorldContext);

    // Move Result pin to 'UTaskFunctionLibrary::CreateTask'
    CallFunction_Result->PinType = ResultPin->PinType; // Copy type so it uses the right actor subclass
    CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CallFunction_Result);
 


    //////////////////////////////////////////////////////////////////////////
    // create 'set var' nodes
 
    // Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
    UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallFunctionNode, TaskNode, CallFunction_Result, GetClassToSpawn());
 
    // Move 'then' connection from create widget node to the last 'then'
    CompilerContext.MovePinLinksToIntermediate(*ThenPin, *LastThen);
 
    // Break any links to the expanded node
    TaskNode->BreakAllNodeLinks();
}
 
#undef LOCTEXT_NAMESPACE