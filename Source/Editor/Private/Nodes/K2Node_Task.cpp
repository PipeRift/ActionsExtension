// Copyright 2015-2017 Piperift. All Rights Reserved.
 
#include "AIExtensionEditorPrivatePCH.h"
 
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CallFunction.h"
#include "K2Node_AssignmentStatement.h"

#include "TaskNodeHelpers.h"

#include "TaskFunctionLibrary.h"
#include "Task.h"

#include "K2Node_Task.h"
 
#define LOCTEXT_NAMESPACE "AIExtensionEditor"


FString UK2Node_Task::FHelper::WorldContextPinName(TEXT("WorldContextObject"));
FString UK2Node_Task::FHelper::ClassPinName(TEXT("Class"));
FString UK2Node_Task::FHelper::OwnerPinName(TEXT("Owner"));



UK2Node_Task::UK2Node_Task(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeTooltip = LOCTEXT("NodeTooltip", "Creates a new task object");
}


///////////////////////////////////////////////////////////////////////////////
// UEdGraphNode Interface

void UK2Node_Task::AllocateDefaultPins()
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    // Add execution pins
    CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
    CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

    // Task Owner
    CreatePin(EGPD_Input, K2Schema->PC_Interface, TEXT(""), UTaskOwnerInterface::StaticClass(), false, false, FHelper::OwnerPinName);

    //If we are not using a predefined class
    if (!UsePrestatedClass()) {
        // Add blueprint pin
        UEdGraphPin* ClassPin = CreatePin(EGPD_Input, K2Schema->PC_Class, TEXT(""), GetClassPinBaseClass(), false, false, FHelper::ClassPinName);
    }

    // Result pin
    UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), GetClassPinBaseClass(), false, false, K2Schema->PN_ReturnValue);

    //Update class pins if we are using a prestated node
    if (UsePrestatedClass())
        OnClassPinChanged();

    Super::AllocateDefaultPins();
}

FLinearColor UK2Node_Task::GetNodeTitleColor() const
{
    return Super::GetNodeTitleColor();
}

FText UK2Node_Task::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (!UsePrestatedClass() && (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle))
    {
        return GetBaseNodeTitle();
    }
    else if (auto ClassToSpawn = GetClassToSpawn())
    {
        if (CachedNodeTitle.IsOutOfDate(this))
        {
            FFormatNamedArguments Args;
            Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());
            // FText::Format() is slow, so we cache this to save on performance
            CachedNodeTitle.SetCachedText(FText::Format(GetNodeTitleFormat(), Args), this);
        }
        return CachedNodeTitle;
    }
    return NSLOCTEXT("K2Node", "ConstructObject_Title_NONE", "Create NONE");
}

void UK2Node_Task::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
    if (ChangedPin && (ChangedPin->PinName == FHelper::ClassPinName))
    {
        OnClassPinChanged();
    }
}

FText UK2Node_Task::GetTooltipText() const
{
    return NodeTooltip;
}

bool UK2Node_Task::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
    UClass* SourceClass = GetClassToSpawn();
    const UBlueprint* SourceBlueprint = GetBlueprint();
    const bool bResult = (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
    if (bResult && OptionalOutput)
    {
        OptionalOutput->AddUnique(SourceClass);
    }
    const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
    return bSuperResult || bResult;
}

bool UK2Node_Task::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
    return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

void UK2Node_Task::PinConnectionListChanged(UEdGraphPin* Pin)
{
    if (Pin && (Pin->PinName == FHelper::ClassPinName))
    {
        OnClassPinChanged();
    }
}

void UK2Node_Task::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
    UEdGraphPin* ClassPin = GetClassPin();
    if (ClassPin)
    {
        SetPinToolTip(*ClassPin, LOCTEXT("ClassPinDescription", "The object class you want to construct"));
    }
    UEdGraphPin* ResultPin = GetResultPin();
    if (ResultPin)
    {
        SetPinToolTip(*ResultPin, LOCTEXT("ResultPinDescription", "The constructed object"));
    }

    if (UEdGraphPin* OwnerPin = GetOwnerPin())
    {
        SetPinToolTip(*OwnerPin, LOCTEXT("OwnerPinDescription", "Owner of the constructed object"));
    }

    return Super::GetPinHoverText(Pin, HoverTextOut);
}

//This will expand node for our custom object, with properties
//which are set as EditAnywhere and meta=(ExposeOnSpawn), or equivalent in blueprint.
void UK2Node_Task::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(SourceGraph && Schema);

    //Get static function
    static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UTaskFunctionLibrary, CreateTask);
    static FName Activate_FunctionName = GET_FUNCTION_NAME_CHECKED(UTask, Activate);

    //Set function parameter names
    static FString ParamName_WidgetType = FString(TEXT("TaskType"));


    /* Retrieve Pins */
    //Exec
    UEdGraphPin* ExecPin = this->GetExecPin();       // Exec pins are those big arrows, connected with thick white lines.   
    UEdGraphPin* ThenPin = this->GetThenPin();       // Then pin is the same as exec pin, just on the other side (the out arrow).

    //Inputs
    UEdGraphPin* OwnerPin = this->GetOwnerPin();
    UEdGraphPin* ClassPin = this->GetClassPin();     // Get class pin which is used to determine which class to spawn.

    //Outputs
    UEdGraphPin* ResultPin = this->GetResultPin();   // Result pin, which will output our spawned object.


                                                         //Don't proceed if OwnerPin is not defined or valid
    if (OwnerPin->LinkedTo.Num() == 0 && OwnerPin->DefaultObject == NULL)
    {
        //TODO: Check if we can connect a self node as the owner.

        CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "Create Task node @@ must have an owner specified.").ToString(), this);
        // we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
        BreakAllNodeLinks();
        return;
    }

    UClass* SpawnClass;
    if(UsePrestatedClass()) {
        SpawnClass = PrestatedClass;
    }
    else
    {
        SpawnClass = ClassPin ? Cast<UClass>(ClassPin->DefaultObject) : NULL;
        //Don't proceed if ClassPin is not defined or valid
        if (ClassPin->LinkedTo.Num() == 0 && NULL == SpawnClass)
        {
            CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "Create Task node @@ must have a class specified.").ToString(), this);
            // we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
            BreakAllNodeLinks();
            return;
        }
    }

    bool bIsErrorFree = true;

    //////////////////////////////////////////////////////////////////////////
    // create 'UTaskFunctionLibrary::CreateTask' call node
    UK2Node_CallFunction* CreateTaskNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    //Attach function
    CreateTaskNode->FunctionReference.SetExternalMember(Create_FunctionName, UTaskFunctionLibrary::StaticClass());
    CreateTaskNode->AllocateDefaultPins();

    //allocate nodes for created widget.
    UEdGraphPin* CreateTask_Exec       = CreateTaskNode->GetExecPin();
    UEdGraphPin* CreateTask_Owner      = CreateTaskNode->FindPinChecked(FHelper::OwnerPinName);
    UEdGraphPin* CreateTask_WidgetType = CreateTaskNode->FindPinChecked(ParamName_WidgetType);
    UEdGraphPin* CreateTask_Result     = CreateTaskNode->GetReturnValuePin();

    // Move 'exec' pin to 'UTaskFunctionLibrary::CreateTask'
    CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CreateTask_Exec);

    //TODO: Create local variable for PrestatedClass

    //Move pin if connected else, copy the value
    if (!UsePrestatedClass() && ClassPin->LinkedTo.Num() > 0)
    {
        CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CreateTask_WidgetType);
    }
    else
    {
        CreateTask_WidgetType->DefaultObject = SpawnClass;
    }

    // Copy Owner pin to 'UTaskFunctionLibrary::CreateTask' if necessary
    if (OwnerPin)
        CompilerContext.MovePinLinksToIntermediate(*OwnerPin, *CreateTask_Owner);

    // Move Result pin to 'UTaskFunctionLibrary::CreateTask'
    CreateTask_Result->PinType = ResultPin->PinType; // Copy type so it uses the right actor subclass
    CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CreateTask_Result);


    //////////////////////////////////////////////////////////////////////////
    // create 'set var' nodes

    // Set all properties of the object
    UEdGraphPin* LastThenPin = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CreateTaskNode, this, CreateTask_Result, GetClassToSpawn());


    // FOR EACH DELEGATE DEFINE EVENT, CONNECT IT TO DELEGATE AND IMPLEMENT A CHAIN OF ASSIGMENTS
    for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(GetClassToSpawn(), EFieldIteratorFlags::ExcludeSuper); PropertyIt && bIsErrorFree; ++PropertyIt)
    {
        UMulticastDelegateProperty* Property = *PropertyIt;

        const UFunction* DelegateSignatureFunction = Property->SignatureFunction;
        if (DelegateSignatureFunction->NumParms < 1) {
            bIsErrorFree &= FHelper::HandleDelegateImplementation(Property, CreateTask_Result, LastThenPin, this, SourceGraph, CompilerContext);
        }
        else
        {
            bIsErrorFree &= FHelper::HandleDelegateBindImplementation(Property, CreateTask_Result, LastThenPin, this, SourceGraph, CompilerContext);
        }
    }

    if (!bIsErrorFree) {
        CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "There was a compile error while binding delegates.").ToString(), this);
        // we break exec links so this is the only error we get, don't want the CreateTask node being considered and giving 'unexpected node' type warnings
        BreakAllNodeLinks();
        return;
    }


    //////////////////////////////////////////////////////////////////////////
    // create 'UTaskFunctionLibrary::ActivateTask' call node
    UK2Node_CallFunction* ActivateTaskNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    //Attach function
    ActivateTaskNode->FunctionReference.SetExternalMember(Activate_FunctionName, UTask::StaticClass());
    ActivateTaskNode->AllocateDefaultPins();

    //allocate nodes for created widget.
    UEdGraphPin* ActivateTask_Exec = ActivateTaskNode->GetExecPin();
    UEdGraphPin* ActivateTask_Self = ActivateTaskNode->FindPinChecked(Schema->PN_Self);
    UEdGraphPin* ActivateTask_Then = ActivateTaskNode->GetThenPin();

    bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateTask_Exec);
    bIsErrorFree &= Schema->TryCreateConnection(CreateTask_Result, ActivateTask_Self);

    CompilerContext.MovePinLinksToIntermediate(*ThenPin, *ActivateTask_Then);


    if (!bIsErrorFree) {
        CompilerContext.MessageLog.Error(*LOCTEXT("CreateTaskNodeMissingClass_Error", "There was a compile error while activating the task.").ToString(), this);
        // we break exec links so this is the only error we get, don't want the CreateTask node being considered and giving 'unexpected node' type warnings
        BreakAllNodeLinks();
        return;
    }

    // Break any links to the expanded node
    BreakAllNodeLinks();
}

///////////////////////////////////////////////////////////////////////////////
// UK2Node Interface

void UK2Node_Task::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    AllocateDefaultPins();
    UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
    if (UseSpawnClass != NULL)
    {
        CreatePinsForClass(UseSpawnClass);
    }
    RestoreSplitPins(OldPins);
}

void UK2Node_Task::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const
{
    UClass* ClassToSpawn = GetClassToSpawn();
    const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
    OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Type"), TEXT("CreateTask")));
    OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
    OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
    OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("ObjectClass"), ClassToSpawnStr));
}

void UK2Node_Task::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    //Registry subclasses creation
    UClass* NodeClass = GetClass();
    TaskNodeHelpers::RegisterTaskClassActions(ActionRegistrar, NodeClass);

    //Registry base creation
    if (ActionRegistrar.IsOpenForRegistration(NodeClass))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(NodeClass);
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(NodeClass, NodeSpawner);
    }
}

//Set context menu category in which our node will be present.
FText UK2Node_Task::GetMenuCategory() const
{
    return FText::FromString("Tasks");
}


///////////////////////////////////////////////////////////////////////////////
// UK2Node_Task

void UK2Node_Task::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
{
    check(InClass != NULL);

    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    const UObject* const ClassDefaultObject = InClass->GetDefaultObject(false);

    
    for (TFieldIterator<UProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        UProperty* Property = *PropertyIt;
        const bool bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
        const bool bIsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
        const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        if (NULL == FindPin(Property->GetName())) {
            if (bIsExposedToSpawn &&
                !Property->HasAnyPropertyFlags(CPF_Parm) &&
                bIsSettableExternally &&
                Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
                !bIsDelegate)
            {
                UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Property->GetName());
                const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);

                if (OutClassPins && Pin)
                {
                    OutClassPins->Add(Pin);
                }

                if (ClassDefaultObject && Pin != NULL && K2Schema->PinDefaultValueIsEditable(*Pin))
                {
                    FString DefaultValueAsString;
                    const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString);
                    check(bDefaultValueSet);
                    K2Schema->TrySetDefaultValue(*Pin, DefaultValueAsString);
                }

                // Copy tooltip from the property.
                if (Pin != nullptr)
                {
                    K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
                }
            }
            else if (bIsDelegate && Property->HasAllPropertyFlags(CPF_BlueprintAssignable)) {
                UMulticastDelegateProperty* const Delegate = Cast<UMulticastDelegateProperty>(*PropertyIt);
                if (Delegate)
                {
                    UFunction* DelegateSignatureFunction = Delegate->SignatureFunction;
                    UEdGraphPin* Pin;
                    if (DelegateSignatureFunction->NumParms < 1)
                    {
                        Pin = CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, Delegate->GetName());
                    }
                    else
                    {
                        Pin = CreatePin(EGPD_Input, K2Schema->PC_Delegate, TEXT(""), NULL, false, true, Delegate->GetName(), true);
                        Pin->PinFriendlyName = FText::Format(NSLOCTEXT("K2Node", "PinFriendlyDelegatetName", "{0} Event"), FText::FromString(Delegate->GetName()));

                        //Update PinType with the delegate's signature
                        FMemberReference::FillSimpleMemberReference<UFunction>(DelegateSignatureFunction, Pin->PinType.PinSubCategoryMemberReference);
                    }

                    if (OutClassPins && Pin)
                    {
                        OutClassPins->Add(Pin);
                    }
                }
            }
        }
    }

    // Change class of output pin
    UEdGraphPin* ResultPin = GetResultPin();
    ResultPin->PinType.PinSubCategoryObject = InClass;
}

bool UK2Node_Task::IsSpawnVarPin(UEdGraphPin* Pin)
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    return(Pin->PinName != K2Schema->PN_Execute &&
        Pin->PinName != K2Schema->PN_Then &&
        Pin->PinName != K2Schema->PN_ReturnValue &&
        Pin->PinName != FHelper::ClassPinName &&
        Pin->PinName != FHelper::WorldContextPinName &&
        Pin->PinName != FHelper::OwnerPinName);
}

UEdGraphPin* UK2Node_Task::GetThenPin()const
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
    check(Pin->Direction == EGPD_Output);
    return Pin;
}

UEdGraphPin* UK2Node_Task::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
    if (UsePrestatedClass())
        return nullptr;

    const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

    UEdGraphPin* Pin = NULL;
    for (auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt)
    {
        UEdGraphPin* TestPin = *PinIt;
        if (TestPin && TestPin->PinName == FHelper::ClassPinName)
        {
            Pin = TestPin;
            break;
        }
    }
    check(Pin == NULL || Pin->Direction == EGPD_Input);
    return Pin;
}

UEdGraphPin* UK2Node_Task::GetWorldContextPin(bool bChecked /*= true*/) const
{
    UEdGraphPin* Pin = FindPin(FHelper::WorldContextPinName);
    if (bChecked) {
        check(Pin == NULL || Pin->Direction == EGPD_Input);
    }
    return Pin;
}

UEdGraphPin* UK2Node_Task::GetResultPin() const
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
    check(Pin->Direction == EGPD_Output);
    return Pin;
}

UEdGraphPin* UK2Node_Task::GetOwnerPin() const
{
    UEdGraphPin* Pin = FindPin(FHelper::OwnerPinName);
    ensure(nullptr == Pin || Pin->Direction == EGPD_Input);
    return Pin;
}

UClass* UK2Node_Task::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
    UClass* UseSpawnClass = NULL;

    if (UsePrestatedClass())
    {
        UseSpawnClass = PrestatedClass;
    }
    else
    {
        const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

        UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
        if (ClassPin && ClassPin->DefaultObject != NULL && ClassPin->LinkedTo.Num() == 0)
        {
            UseSpawnClass = CastChecked<UClass>(ClassPin->DefaultObject);
        }
        else if (ClassPin && ClassPin->LinkedTo.Num())
        {
            auto ClassSource = ClassPin->LinkedTo[0];
            UseSpawnClass = ClassSource ? Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get()) : nullptr;
        }
    }

    return UseSpawnClass;
}

bool UK2Node_Task::UseWorldContext() const
{
    auto BP = GetBlueprint();
    const UClass* ParentClass = BP ? BP->ParentClass : nullptr;
    return ParentClass ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

FText UK2Node_Task::GetBaseNodeTitle() const
{
    return LOCTEXT("BTNode_BaseTitle", "Create Task");
}

FText UK2Node_Task::GetNodeTitleFormat() const
{
    return LOCTEXT("BTTask", "Create {ClassName}");
}

//which class can be used with this node to create objects. All childs of class can be used.
UClass* UK2Node_Task::GetClassPinBaseClass() const
{
    return UsePrestatedClass() ? PrestatedClass : UTask::StaticClass();
}

void UK2Node_Task::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
{
    MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToText(MutatablePin.PinType).ToString();

    UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
    if (K2Schema != nullptr)
    {
        MutatablePin.PinToolTip += TEXT(" ");
        MutatablePin.PinToolTip += K2Schema->GetPinDisplayName(&MutatablePin).ToString();
    }

    MutatablePin.PinToolTip += FString(TEXT("\n")) + PinDescription.ToString();
}

void UK2Node_Task::OnClassPinChanged()
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    // Remove all pins related to archetype variables
    TArray<UEdGraphPin*> OldPins = Pins;
    TArray<UEdGraphPin*> OldClassPins;

    for (int32 i = 0; i < OldPins.Num(); i++)
    {
        UEdGraphPin* OldPin = OldPins[i];
        if (IsSpawnVarPin(OldPin))
        {
            OldPin->MarkPendingKill();
            Pins.Remove(OldPin);
            OldClassPins.Add(OldPin);
        }
    }

    CachedNodeTitle.MarkDirty();

    UClass* UseSpawnClass = GetClassToSpawn();
    TArray<UEdGraphPin*> NewClassPins;
    if (UseSpawnClass != NULL)
    {
        CreatePinsForClass(UseSpawnClass, &NewClassPins);
    }

    // Rewire the old pins to the new pins so connections are maintained if possible
    RewireOldPinsToNewPins(OldClassPins, NewClassPins);

    // Destroy the old pins
    DestroyPinList(OldClassPins);

    // Refresh the UI for the graph so the pin changes show up
    UEdGraph* Graph = GetGraph();
    Graph->NotifyGraphChanged();

    // Mark dirty
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

bool UK2Node_Task::FHelper::ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction, const UEdGraphSchema_K2* Schema)
{
    check(Schema);
    const bool bValidDataPin = Pin
        && (Pin->PinName != Schema->PN_Execute)
        && (Pin->PinName != Schema->PN_Then)
        && (Pin->PinType.PinCategory != Schema->PC_Exec);

    const bool bProperDirection = Pin && (Pin->Direction == Direction);

    return bValidDataPin && bProperDirection;
}

bool UK2Node_Task::FHelper::CreateDelegateForNewFunction(UEdGraphPin* DelegateInputPin, FName FunctionName, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
{
    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(DelegateInputPin && Schema && CurrentNode && SourceGraph && (FunctionName != NAME_None));
    bool bResult = true;

    // WORKAROUND, so we can create delegate from nonexistent function by avoiding check at expanding step
    // instead simply: Schema->TryCreateConnection(AddDelegateNode->GetDelegatePin(), CurrentCENode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName));
    UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(CurrentNode, SourceGraph);
    SelfNode->AllocateDefaultPins();

    UK2Node_CreateDelegate* CreateDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(CurrentNode, SourceGraph);
    CreateDelegateNode->AllocateDefaultPins();
    bResult &= Schema->TryCreateConnection(DelegateInputPin, CreateDelegateNode->GetDelegateOutPin());
    bResult &= Schema->TryCreateConnection(SelfNode->FindPinChecked(Schema->PN_Self), CreateDelegateNode->GetObjectInPin());
    CreateDelegateNode->SetFunction(FunctionName);

    return bResult;
}

bool UK2Node_Task::FHelper::CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
{
    check(CENode && Function && Schema);

    bool bResult = true;
    for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
    {
        const UProperty* Param = *PropIt;
        if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
        {
            FEdGraphPinType PinType;
            bResult &= Schema->ConvertPropertyToPinType(Param, /*out*/ PinType);
            bResult &= (NULL != CENode->CreateUserDefinedPin(Param->GetName(), PinType, EGPD_Output));
        }
    }
    return bResult;
}

bool UK2Node_Task::FHelper::HandleDelegateImplementation(
    UMulticastDelegateProperty* CurrentProperty,
    UEdGraphPin* ProxyObjectPin, UEdGraphPin*& InOutLastThenPin,
    UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
{
    bool bIsErrorFree = true;
    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(CurrentProperty && ProxyObjectPin && InOutLastThenPin && CurrentNode && SourceGraph && Schema);

    UEdGraphPin* PinForCurrentDelegateProperty = CurrentNode->FindPin(CurrentProperty->GetName());
    if (!PinForCurrentDelegateProperty || (Schema->PC_Exec != PinForCurrentDelegateProperty->PinType.PinCategory))
    {
        FText ErrorMessage = FText::Format(LOCTEXT("WrongDelegateProperty", "BaseAsyncConstructObject: Cannot find execution pin for delegate "), FText::FromString(CurrentProperty->GetName()));
        CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), CurrentNode);
        return false;
    }

    UK2Node_CustomEvent* CurrentCENode = CompilerContext.SpawnIntermediateEventNode<UK2Node_CustomEvent>(CurrentNode, PinForCurrentDelegateProperty, SourceGraph);
    {
        UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);

        UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(CurrentNode);
        bool const bIsSelfContext = Blueprint->SkeletonGeneratedClass->IsChildOf(CurrentProperty->GetOwnerClass());

        AddDelegateNode->SetFromProperty(CurrentProperty, bIsSelfContext);
        AddDelegateNode->AllocateDefaultPins();

        bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(Schema->PN_Self), ProxyObjectPin);
        bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(Schema->PN_Execute));
        InOutLastThenPin = AddDelegateNode->FindPinChecked(Schema->PN_Then);
        CurrentCENode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CompilerContext.GetGuid(CurrentNode));
        CurrentCENode->AllocateDefaultPins();

        bIsErrorFree &= FHelper::CreateDelegateForNewFunction(AddDelegateNode->GetDelegatePin(), CurrentCENode->GetFunctionName(), CurrentNode, SourceGraph, CompilerContext);
        bIsErrorFree &= FHelper::CopyEventSignature(CurrentCENode, AddDelegateNode->GetDelegateSignature(), Schema);
    }

    UEdGraphPin* LastActivatedNodeThen = CurrentCENode->FindPinChecked(Schema->PN_Then);

    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PinForCurrentDelegateProperty, *LastActivatedNodeThen).CanSafeConnect();
    return bIsErrorFree;
}


bool UK2Node_Task::FHelper::HandleDelegateBindImplementation(
    UMulticastDelegateProperty* CurrentProperty,
    UEdGraphPin* ObjectPin, UEdGraphPin*& InOutLastThenPin,
    UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
{
    bool bIsErrorFree = true;
    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

    check(CurrentProperty && Schema);
    UEdGraphPin* DelegateRefPin = CurrentNode->FindPinChecked(CurrentProperty->GetName());
    check(DelegateRefPin);

    UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);
    check(AddDelegateNode);


    AddDelegateNode->SetFromProperty(CurrentProperty, false);
    AddDelegateNode->AllocateDefaultPins();

    bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(Schema->PN_Self), ObjectPin);
    bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(Schema->PN_Execute));
    InOutLastThenPin = AddDelegateNode->FindPinChecked(Schema->PN_Then);

    UEdGraphPin* AddDelegate_DelegatePin = AddDelegateNode->GetDelegatePin();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*DelegateRefPin, *AddDelegate_DelegatePin).CanSafeConnect();
    DelegateRefPin->PinType = AddDelegate_DelegatePin->PinType;

    return bIsErrorFree;
}

#undef LOCTEXT_NAMESPACE