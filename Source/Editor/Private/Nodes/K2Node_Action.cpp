// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "K2Node_Action.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CallFunction.h"
#include "K2Node_AssignmentStatement.h"

#include "ActionNodeHelpers.h"

#include "ActionFunctionLibrary.h"
#include "Action.h"


#define LOCTEXT_NAMESPACE "AIExtensionEditor"

FName UK2Node_Action::FHelper::WorldContextPinName(TEXT("WorldContextObject"));
FName UK2Node_Action::FHelper::ClassPinName(TEXT("Class"));
FName UK2Node_Action::FHelper::OwnerPinName(TEXT("Owner"));



UK2Node_Action::UK2Node_Action(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Creates a new action");
}


///////////////////////////////////////////////////////////////////////////////
// UEdGraphNode Interface

void UK2Node_Action::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, K2Schema->PC_Exec, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, K2Schema->PN_Then);

	// Action Owner
	CreatePin(EGPD_Input, K2Schema->PC_Interface, UActionOwnerInterface::StaticClass(), FHelper::OwnerPinName);

	//If we are not using a predefined class
	if (!UsePrestatedClass()) {
		// Add blueprint pin
		UEdGraphPin* ClassPin = CreatePin(EGPD_Input, K2Schema->PC_Class, GetClassPinBaseClass(), FHelper::ClassPinName);
	}

	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, GetClassPinBaseClass(), K2Schema->PN_ReturnValue);

	//Update class pins if we are using a prestated node
	if (UsePrestatedClass())
		OnClassPinChanged();

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_Action::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

FText UK2Node_Action::GetNodeTitle(ENodeTitleType::Type TitleType) const
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

void UK2Node_Action::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	if (ChangedPin && (ChangedPin->PinName == FHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

FText UK2Node_Action::GetTooltipText() const
{
	return NodeTooltip;
}

bool UK2Node_Action::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
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

bool UK2Node_Action::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

void UK2Node_Action::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin && (Pin->PinName == FHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

void UK2Node_Action::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
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
void UK2Node_Action::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(SourceGraph && Schema);

	//Get static function
	static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UActionFunctionLibrary, CreateAction);
	static FName Activate_FunctionName = GET_FUNCTION_NAME_CHECKED(UAction, Activate);

	//Set function parameter names
	static FString ParamName_WidgetType = FString(TEXT("Type"));


	/* Retrieve Pins */
	//Exec
	UEdGraphPin* ExecPin = this->GetExecPin();	   // Exec pins are those big arrows, connected with thick white lines.
	UEdGraphPin* ThenPin = this->GetThenPin();	   // Then pin is the same as exec pin, just on the other side (the out arrow).

	//Inputs
	UEdGraphPin* OwnerPin = this->GetOwnerPin();
	UEdGraphPin* ClassPin = this->GetClassPin();	 // Get class pin which is used to determine which class to spawn.

	//Outputs
	UEdGraphPin* ResultPin = this->GetResultPin();   // Result pin, which will output our spawned object.


														 //Don't proceed if OwnerPin is not defined or valid
	if (OwnerPin->LinkedTo.Num() == 0 && OwnerPin->DefaultObject == NULL)
	{
		//TODO: Check if we can connect a self node as the owner.

		CompilerContext.MessageLog.Error(*LOCTEXT("CreateActionNodeMissingClass_Error", "Create Action node @@ must have an owner specified.").ToString(), this);
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
			CompilerContext.MessageLog.Error(*LOCTEXT("CreateActionNodeMissingClass_Error", "Create Action node @@ must have a class specified.").ToString(), this);
			// we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
			BreakAllNodeLinks();
			return;
		}
	}

	bool bIsErrorFree = true;

	//////////////////////////////////////////////////////////////////////////
	// create 'UActionFunctionLibrary::CreateAction' call node
	UK2Node_CallFunction* CreateActionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	//Attach function
	CreateActionNode->FunctionReference.SetExternalMember(Create_FunctionName, UActionFunctionLibrary::StaticClass());
	CreateActionNode->AllocateDefaultPins();

	//allocate nodes for created widget.
	UEdGraphPin* CreateAction_Exec	   = CreateActionNode->GetExecPin();
	UEdGraphPin* CreateAction_Owner	  = CreateActionNode->FindPinChecked(FHelper::OwnerPinName);
	UEdGraphPin* CreateAction_WidgetType = CreateActionNode->FindPinChecked(ParamName_WidgetType);
	UEdGraphPin* CreateAction_Result	 = CreateActionNode->GetReturnValuePin();

	// Move 'exec' pin to 'UActionFunctionLibrary::CreateAction'
	CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CreateAction_Exec);

	//TODO: Create local variable for PrestatedClass

	//Move pin if connected else, copy the value
	if (!UsePrestatedClass() && ClassPin->LinkedTo.Num() > 0)
	{
		CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CreateAction_WidgetType);
	}
	else
	{
		CreateAction_WidgetType->DefaultObject = SpawnClass;
	}

	// Copy Owner pin to 'UActionFunctionLibrary::CreateAction' if necessary
	if (OwnerPin)
		CompilerContext.MovePinLinksToIntermediate(*OwnerPin, *CreateAction_Owner);

	// Move Result pin to 'UActionFunctionLibrary::CreateAction'
	CreateAction_Result->PinType = ResultPin->PinType; // Copy type so it uses the right actor subclass
	CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CreateAction_Result);


	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes

	// Set all properties of the object
	UEdGraphPin* LastThenPin = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CreateActionNode, this, CreateAction_Result, GetClassToSpawn());


	// FOR EACH DELEGATE DEFINE EVENT, CONNECT IT TO DELEGATE AND IMPLEMENT A CHAIN OF ASSIGMENTS
	for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(GetClassToSpawn(), EFieldIteratorFlags::IncludeSuper); PropertyIt && bIsErrorFree; ++PropertyIt)
	{
		UMulticastDelegateProperty* Property = *PropertyIt;

		if (Property && Property->HasAllPropertyFlags(CPF_BlueprintAssignable))
		{
			const UFunction* DelegateSignatureFunction = Property->SignatureFunction;
			if (DelegateSignatureFunction->NumParms < 1)
			{
				bIsErrorFree &= FHelper::HandleDelegateImplementation(Property, CreateAction_Result, LastThenPin, this, SourceGraph, CompilerContext);
			}
			else
			{
				bIsErrorFree &= FHelper::HandleDelegateBindImplementation(Property, CreateAction_Result, LastThenPin, this, SourceGraph, CompilerContext);
			}
		}
	}

	if (!bIsErrorFree) {
		CompilerContext.MessageLog.Error(*LOCTEXT("CreateActionNodeMissingClass_Error", "There was a compile error while binding delegates.").ToString(), this);
		// we break exec links so this is the only error we get, don't want the CreateAction node being considered and giving 'unexpected node' type warnings
		BreakAllNodeLinks();
		return;
	}


	//////////////////////////////////////////////////////////////////////////
	// create 'UAction::Activate' call node
	UK2Node_CallFunction* ActivateActionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	//Attach function
	ActivateActionNode->FunctionReference.SetExternalMember(Activate_FunctionName, UAction::StaticClass());
	ActivateActionNode->AllocateDefaultPins();

	//allocate nodes for created widget.
	UEdGraphPin* ActivateAction_Exec = ActivateActionNode->GetExecPin();
	UEdGraphPin* ActivateAction_Self = ActivateActionNode->FindPinChecked(Schema->PN_Self);
	UEdGraphPin* ActivateAction_Then = ActivateActionNode->GetThenPin();

	bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateAction_Exec);
	bIsErrorFree &= Schema->TryCreateConnection(CreateAction_Result, ActivateAction_Self);

	CompilerContext.MovePinLinksToIntermediate(*ThenPin, *ActivateAction_Then);


	if (!bIsErrorFree) {
		CompilerContext.MessageLog.Error(*LOCTEXT("CreateActionNodeMissingClass_Error", "There was a compile error while activating the action.").ToString(), this);
		// we break exec links so this is the only error we get, don't want the CreateAction node being considered and giving 'unexpected node' type warnings
		BreakAllNodeLinks();
		return;
	}

	// Break any links to the expanded node
	BreakAllNodeLinks();
}

///////////////////////////////////////////////////////////////////////////////
// UK2Node Interface

void UK2Node_Action::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();
	UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
	if (UseSpawnClass != NULL)
	{
		CreatePinsForClass(UseSpawnClass);
	}
	RestoreSplitPins(OldPins);
}

void UK2Node_Action::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const
{
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Type"), TEXT("CreateAction")));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("ObjectClass"), ClassToSpawnStr));
}

void UK2Node_Action::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	//Registry subclasses creation
	UClass* NodeClass = GetClass();
	ActionNodeHelpers::RegisterActionClassActions(ActionRegistrar, NodeClass);

	//Registry base creation
	if (ActionRegistrar.IsOpenForRegistration(NodeClass))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(NodeClass);
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(NodeClass, NodeSpawner);
	}
}

//Set context menu category in which our node will be present.
FText UK2Node_Action::GetMenuCategory() const
{
	return FText::FromString("Actions");
}


///////////////////////////////////////////////////////////////////////////////
// UK2Node_Action

void UK2Node_Action::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
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
				UEdGraphPin* Pin = CreatePin(EGPD_Input, FName(), Property->GetFName());
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
						Pin = CreatePin(EGPD_Output, K2Schema->PC_Exec, Delegate->GetFName());
					}
					else
					{
						UEdGraphNode::FCreatePinParams Params{};
						Params.bIsConst = true;
						Params.bIsReference = true;

						Pin = CreatePin(EGPD_Input, K2Schema->PC_Delegate, Delegate->GetFName(), Params);
						Pin->PinFriendlyName = FText::Format(NSLOCTEXT("K2Node", "PinFriendlyDelegatetName", "{0} Event"), FText::FromName(Delegate->GetFName()));

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

bool UK2Node_Action::IsSpawnVarPin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	return(Pin->PinName != K2Schema->PN_Execute &&
		Pin->PinName != K2Schema->PN_Then &&
		Pin->PinName != K2Schema->PN_ReturnValue &&
		Pin->PinName != FHelper::ClassPinName &&
		Pin->PinName != FHelper::WorldContextPinName &&
		Pin->PinName != FHelper::OwnerPinName);
}

UEdGraphPin* UK2Node_Action::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_Action::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
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

UEdGraphPin* UK2Node_Action::GetWorldContextPin(bool bChecked /*= true*/) const
{
	UEdGraphPin* Pin = FindPin(FHelper::WorldContextPinName);
	if (bChecked) {
		check(Pin == NULL || Pin->Direction == EGPD_Input);
	}
	return Pin;
}

UEdGraphPin* UK2Node_Action::GetResultPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_Action::GetOwnerPin() const
{
	UEdGraphPin* Pin = FindPin(FHelper::OwnerPinName);
	ensure(nullptr == Pin || Pin->Direction == EGPD_Input);
	return Pin;
}

UClass* UK2Node_Action::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
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

bool UK2Node_Action::UseWorldContext() const
{
	auto BP = GetBlueprint();
	const UClass* ParentClass = BP ? BP->ParentClass : nullptr;
	return ParentClass ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

FText UK2Node_Action::GetBaseNodeTitle() const
{
	return LOCTEXT("Action_BaseTitle", "Create Action");
}

FText UK2Node_Action::GetNodeTitleFormat() const
{
	return LOCTEXT("Action_ClassTitle", "{ClassName}");
}

//which class can be used with this node to create objects. All childs of class can be used.
UClass* UK2Node_Action::GetClassPinBaseClass() const
{
	return UsePrestatedClass() ? PrestatedClass : UAction::StaticClass();
}

void UK2Node_Action::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
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

void UK2Node_Action::OnClassPinChanged()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Remove all pins related to archetype variables
	TArray<UEdGraphPin*> OldPins = Pins;
	TArray<UEdGraphPin*> OldClassPins;


	for (UEdGraphPin* OldPin : OldPins)
	{
		if (IsSpawnVarPin(OldPin))
		{
			Pins.Remove(OldPin);
			OldClassPins.Add(OldPin);
		}
	}

	CachedNodeTitle.MarkDirty();

	TArray<UEdGraphPin*> NewClassPins;
	if (UClass* UseSpawnClass = GetClassToSpawn())
	{
		CreatePinsForClass(UseSpawnClass, &NewClassPins);
	}

	RestoreSplitPins(OldPins);

	UEdGraphPin* ResultPin = GetResultPin();
	// Cache all the pin connections to the ResultPin, we will attempt to recreate them
	TArray<UEdGraphPin*> ResultPinConnectionList = ResultPin->LinkedTo;
	// Because the archetype has changed, we break the output link as the output pin type will change
	ResultPin->BreakAllPinLinks(true);

	// Recreate any pin links to the Result pin that are still valid
	for (UEdGraphPin* Connections : ResultPinConnectionList)
	{
		K2Schema->TryCreateConnection(ResultPin, Connections);
	}

	// Rewire the old pins to the new pins so connections are maintained if possible
	RewireOldPinsToNewPins(OldClassPins, Pins);

	// Refresh the UI for the graph so the pin changes show up
	GetGraph()->NotifyGraphChanged();

	// Mark dirty
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

bool UK2Node_Action::FHelper::ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction, const UEdGraphSchema_K2* Schema)
{
	check(Schema);
	const bool bValidDataPin = Pin
		&& (Pin->PinName != Schema->PN_Execute)
		&& (Pin->PinName != Schema->PN_Then)
		&& (Pin->PinType.PinCategory != Schema->PC_Exec);

	const bool bProperDirection = Pin && (Pin->Direction == Direction);

	return bValidDataPin && bProperDirection;
}

bool UK2Node_Action::FHelper::CreateDelegateForNewFunction(UEdGraphPin* DelegateInputPin, FName FunctionName, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
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

bool UK2Node_Action::FHelper::CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
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
			bResult &= (NULL != CENode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output));
		}
	}
	return bResult;
}

bool UK2Node_Action::FHelper::HandleDelegateImplementation(
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


bool UK2Node_Action::FHelper::HandleDelegateBindImplementation(
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