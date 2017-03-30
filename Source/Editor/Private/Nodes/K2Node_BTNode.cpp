// Copyright 2015-2017 Piperift. All Rights Reserved.
 
#include "AIExtensionEditorPrivatePCH.h"
 
#include "TaskFunctionLibrary.h"
#include "Task.h"
 
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
    return UTask::StaticClass();
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

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(SourceGraph && Schema);

    //Get static function
    static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UTaskFunctionLibrary, CreateTask);
    static FName Activate_FunctionName = GET_FUNCTION_NAME_CHECKED(UTaskFunctionLibrary, ActivateTask);

    //Set function parameter names
    static FString ParamName_WorldContextObject = FString(TEXT("WorldContextObject"));
    static FString ParamName_WidgetType = FString(TEXT("TaskType"));

 
    /* Retrieve Pins */
    //Exec
    UEdGraphPin* ExecPin         = this->GetExecPin();         // Exec pins are those big arrows, connected with thick white lines.   
    UEdGraphPin* ThenPin         = this->GetThenPin();         // Then pin is the same as exec pin, just on the other side (the out arrow).
    //Inputs
    UEdGraphPin* WorldContextPin = this->GetWorldContextPin(); // Gets world context pin from our static function
    UEdGraphPin* ClassPin        = this->GetClassPin();        // Get class pin which is used to determine which class to spawn.
    //Outputs
    UEdGraphPin* ResultPin       = this->GetResultPin();       // Result pin, which will output our spawned object.



    UClass* SpawnClass = ClassPin ? Cast<UClass>(ClassPin->DefaultObject) : NULL;

    //Don't proceed if ClassPin is not defined or valid
    if (ClassPin->LinkedTo.Num() == 0 && NULL == SpawnClass)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "Spawn node @@ must have a class specified.").ToString(), this);
        // we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
        BreakAllNodeLinks();
        return;
    }

    bool bError = false;
 
    //////////////////////////////////////////////////////////////////////////
    // create 'UTaskFunctionLibrary::CreateTask' call node
    UK2Node_CallFunction* CreateTaskNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    //Attach function
    CreateTaskNode->FunctionReference.SetExternalMember(Create_FunctionName, UTaskFunctionLibrary::StaticClass());
    CreateTaskNode->AllocateDefaultPins();
 
    //allocate nodes for created widget.
    UEdGraphPin* CreateTask_Exec = CreateTaskNode->GetExecPin();
    UEdGraphPin* CreateTask_WorldContext = CreateTaskNode->FindPinChecked(ParamName_WorldContextObject);
    UEdGraphPin* CreateTask_WidgetType = CreateTaskNode->FindPinChecked(ParamName_WidgetType);
    UEdGraphPin* CreateTask_Result = CreateTaskNode->GetReturnValuePin();
 
    // Move 'exec' pin to 'UTaskFunctionLibrary::CreateTask'
    CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CreateTask_Exec);
 
    //Move pin if connected else, copy the value
    if (ClassPin->LinkedTo.Num() > 0) {
        CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CreateTask_WidgetType);
    } else {
        CreateTask_WidgetType->DefaultObject = SpawnClass;
    }
 
    // Copy WorldContext pin to 'UTaskFunctionLibrary::CreateTask' if necessary
    if (WorldContextPin)
        CompilerContext.MovePinLinksToIntermediate(*WorldContextPin, *CreateTask_WorldContext);

    // Move Result pin to 'UTaskFunctionLibrary::CreateTask'
    CreateTask_Result->PinType = ResultPin->PinType; // Copy type so it uses the right actor subclass
    CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CreateTask_Result);



    //////////////////////////////////////////////////////////////////////////
    // create 'set var' nodes
 
    // Set all properties of the object
    UEdGraphPin* LastThenPin = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CreateTaskNode, this, CreateTask_Result, GetClassToSpawn());


    CompilerContext.MovePinLinksToIntermediate(*ThenPin, *LastThenPin);


    if (bError) {
        CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "There was a compile error.").ToString(), this);
        // we break exec links so this is the only error we get, don't want the CreateTask node being considered and giving 'unexpected node' type warnings
        BreakAllNodeLinks();
        return;
    }

    // Break any links to the expanded node
    BreakAllNodeLinks();
}
 
#undef LOCTEXT_NAMESPACE