// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "K2Node_Action.h"

#include <BlueprintNodeSpawner.h>
#include <BlueprintFunctionNodeSpawner.h>
#include <BlueprintActionDatabaseRegistrar.h>
#include <EditorCategoryUtils.h>
#include <K2Node_CallFunction.h>
#include <K2Node_AssignmentStatement.h>
#include <KismetCompiler.h>
#include <Kismet2/BlueprintEditorUtils.h>

#include "ActionsEditor.h"
#include "ActionReflection.h"
#include "ActionNodeHelpers.h"

#include "ActionLibrary.h"
#include "Action.h"


#define LOCTEXT_NAMESPACE "ActionEditor"

const FName UK2Node_Action::ClassPinName{ "Class" };
const FName UK2Node_Action::OwnerPinName{ UEdGraphSchema_K2::PN_Self };


UK2Node_Action::UK2Node_Action()
	: Super()
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Creates a new action");
}


void UK2Node_Action::PostLoad()
{
	Super::PostLoad();
	BindBlueprintCompile();
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
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, UObject::StaticClass(), OwnerPinName);
	SelfPin->PinFriendlyName = LOCTEXT("Owner", "Owner");


	//If we are not using a predefined class
	if (!UsePrestatedClass()) {
		// Add blueprint pin
		UEdGraphPin* ClassPin = CreatePin(EGPD_Input, K2Schema->PC_Class, GetClassPinBaseClass(), ClassPinName);
	}

	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, GetClassPinBaseClass(), K2Schema->PN_ReturnValue);

	//Update class pins if we are using a prestated node
	if (UsePrestatedClass())
	{
		OnClassPinChanged();
		BindBlueprintCompile();
	}

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_Action::GetNodeTitleColor() const
{
	return FColor{ 27, 240, 247 };
}

FText UK2Node_Action::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!UsePrestatedClass() && (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle))
	{
		return GetBaseNodeTitle();
	}
	else if (auto ClassToSpawn = GetActionClass())
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
	return NSLOCTEXT("K2Node", "Action_Title_None", "No Action");
}

void UK2Node_Action::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	// Class changed
	if (ChangedPin && (ChangedPin->PinName == ClassPinName))
	{
		OnClassPinChanged();
		BindBlueprintCompile();
	}
}

FText UK2Node_Action::GetTooltipText() const
{
	return NodeTooltip;
}

bool UK2Node_Action::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	UClass* SourceClass = GetActionClass();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	const bool bResult = (SourceClass != nullptr) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
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
	if (Pin && (Pin->PinName == ClassPinName))
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
		SetPinToolTip(*ResultPin, LOCTEXT("ResultPinDescription", "The constructed action"));
	}

	if (UEdGraphPin* OwnerPin = GetOwnerPin())
	{
		SetPinToolTip(*OwnerPin, LOCTEXT("OwnerPinDescription", "Parent of the action"));
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
	static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UActionLibrary, CreateAction);
	static FName Activate_FunctionName = GET_FUNCTION_NAME_CHECKED(UAction, Activate);

	//Set function parameter names
	static FString ParamName_Type = FString(TEXT("Type"));


	/* Retrieve Pins */
	//Exec
	UEdGraphPin* ExecPin = GetExecPin();	   // Exec pins are those big arrows, connected with thick white lines.
	UEdGraphPin* ThenPin = GetThenPin();	   // Then pin is the same as exec pin, just on the other side (the out arrow).

	//Inputs
	UEdGraphPin* OwnerPin = GetOwnerPin();
	UEdGraphPin* ClassPin = GetClassPin();	 // Get class pin which is used to determine which class to spawn.

	//Outputs
	UEdGraphPin* ResultPin = GetResultPin();   // Result pin, which will output our spawned object.

	if (!HasWorldContext())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("CreateActionNodeMissingClass_Error", "@@ node requires world context.").ToString(), this);
		// we break exec links so this is the only error we get, don't want the CreateItemData node being considered and giving 'unexpected node' type warnings
		BreakAllNodeLinks();
		return;
	}

	UClass* SpawnClass;
	if (UsePrestatedClass())
	{
		SpawnClass = PrestatedClass;
	}
	else
	{
		SpawnClass = ClassPin ? Cast<UClass>(ClassPin->DefaultObject) : nullptr;
		//Don't proceed if ClassPin is not defined or valid
		if (!ClassPin || (ClassPin->LinkedTo.Num() == 0 && nullptr == SpawnClass))
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
	CreateActionNode->FunctionReference.SetExternalMember(Create_FunctionName, UActionLibrary::StaticClass());
	CreateActionNode->AllocateDefaultPins();

	//allocate nodes for created widget.
	UEdGraphPin* CreateAction_Exec   = CreateActionNode->GetExecPin();
	UEdGraphPin* CreateAction_Owner	 = CreateActionNode->FindPinChecked(TEXT("Owner"));
	UEdGraphPin* CreateAction_Type   = CreateActionNode->FindPinChecked(ParamName_Type);
	UEdGraphPin* CreateAction_Result = CreateActionNode->GetReturnValuePin();

	// Move 'exec' pin to 'UActionFunctionLibrary::CreateAction'
	CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CreateAction_Exec);

	//TODO: Create local variable for PrestatedClass

	//Move pin if connected else, copy the value
	if (!UsePrestatedClass() && ClassPin->LinkedTo.Num() > 0)
	{
		// Copy the 'blueprint' connection from the spawn node to 'UActionLibrary::CreateAction'
		CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CreateAction_Type);
	}
	else
	{
		// Copy blueprint literal onto 'UActionLibrary::CreateAction' call
		CreateAction_Type->DefaultObject = SpawnClass;
	}

	// Copy Owner pin to 'UActionFunctionLibrary::CreateAction' if necessary
	if (OwnerPin)
	{
		CompilerContext.MovePinLinksToIntermediate(*OwnerPin, *CreateAction_Owner);
	}

	// Move Result pin to 'UActionFunctionLibrary::CreateAction'
	CreateAction_Result->PinType = ResultPin->PinType; // Copy type so it uses the right actor subclass
	CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CreateAction_Result);


	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes

	// Set all properties of the object
	UEdGraphPin* LastThenPin = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CreateActionNode, this, CreateAction_Result, GetActionClass());

	// For each delegate, define an event, bind it to delegate and implement a chain of assignments
	for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(GetActionClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt && bIsErrorFree; ++PropertyIt)
	{
		UMulticastDelegateProperty* Property = *PropertyIt;

		if (Property && Property->HasAllPropertyFlags(CPF_BlueprintAssignable) && Property->SignatureFunction)
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

	if (!bIsErrorFree)
	{
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


	if (!bIsErrorFree)
	{
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
	if (UClass* UseSpawnClass = GetActionClass(&OldPins))
	{
		CreatePinsForClass(UseSpawnClass);
	}
	RestoreSplitPins(OldPins);
}

void UK2Node_Action::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const
{
	UClass* ClassToSpawn = GetActionClass();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Type"), TEXT("CreateAction")));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("ObjectClass"), ClassToSpawnStr));
}

void UK2Node_Action::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();

	FActionNodeHelpers::RegisterActionClassActions(ActionRegistrar, ActionKey);

	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		NodeSpawner->DefaultMenuSignature.Keywords = LOCTEXT("MenuKeywords", "Create Action");
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
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
	check(InClass != nullptr);

	auto Properties = ActionReflection::GetVisibleProperties(InClass);
	if (!Properties)
	{
		return;
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	const UObject* const CDO = InClass->GetDefaultObject(false);
	for(const auto& Property : Properties->Variables)
	{
		UEdGraphPin* Pin = CreatePin(EGPD_Input, FName(), Property.GetFName());
		K2Schema->ConvertPropertyToPinType(Property.GetProperty(), Pin->PinType);

		if (OutClassPins)
		{
			OutClassPins->Add(Pin);
		}

		if (CDO && K2Schema->PinDefaultValueIsEditable(*Pin))
		{
			FString DefaultValueAsString;
			const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property.GetProperty(), reinterpret_cast<const uint8*>(CDO), DefaultValueAsString);
			check(bDefaultValueSet);
			K2Schema->TrySetDefaultValue(*Pin, DefaultValueAsString);
		}

		// Copy tooltip from the property.
		K2Schema->ConstructBasicPinTooltip(*Pin, Property.GetProperty()->GetToolTipText(), Pin->PinToolTip);
	}

	for(const auto& Property : Properties->ComplexDelegates)
	{
		if (!Property.GetFunction())
		{
			UE_LOG(LogActionsEd, Error, TEXT("Delegate '%s' may be corrupted"), *Property.GetFName().ToString());
		}

		UEdGraphNode::FCreatePinParams Params{};
		Params.bIsConst = true;
		Params.bIsReference = true;
		UEdGraphPin* Pin = CreatePin(EGPD_Input, K2Schema->PC_Delegate, Property.GetFName(), Params);
		Pin->PinFriendlyName = FText::Format(NSLOCTEXT("K2Node", "PinFriendlyDelegatetName", "{0} Event"), FText::FromName(Property.GetFName()));

		//Update PinType with the delegate's signature
		FMemberReference::FillSimpleMemberReference<UFunction>(Property.GetFunction(), Pin->PinType.PinSubCategoryMemberReference);

		if (OutClassPins)
		{
			OutClassPins->Add(Pin);
		}
	}

	for(const auto& Property : Properties->SimpleDelegates)
	{
		if (!Property.GetFunction())
		{
			UE_LOG(LogActionsEd, Error, TEXT("Delegate '%s' may be corrupted"), *Property.GetFName().ToString());
		}

		UEdGraphPin* Pin = CreatePin(EGPD_Output, K2Schema->PC_Exec, Property.GetFName());
		if (OutClassPins)
		{
			OutClassPins->Add(Pin);
		}
	}

	// Change class of output pin
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass;
}

bool UK2Node_Action::IsActionVarPin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	return(Pin->PinName != K2Schema->PN_Execute &&
		Pin->PinName != K2Schema->PN_Then &&
		Pin->PinName != K2Schema->PN_ReturnValue &&
		Pin->PinName != ClassPinName &&
		Pin->PinName != OwnerPinName);
}

UEdGraphPin* UK2Node_Action::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_Action::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= nullptr*/) const
{
	if (UsePrestatedClass())
		return nullptr;

	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = nullptr;
	for (auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* TestPin = *PinIt;
		if (TestPin && TestPin->PinName == ClassPinName)
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
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
	UEdGraphPin* Pin = FindPin(OwnerPinName);
	ensure(nullptr == Pin || Pin->Direction == EGPD_Input);
	return Pin;
}

UClass* UK2Node_Action::GetActionClass(const TArray<UEdGraphPin*>* InPinsToSearch /*=nullptr*/) const
{
	UClass* UseSpawnClass = nullptr;

	if (UsePrestatedClass())
	{
		UseSpawnClass = PrestatedClass;
	}
	else
	{
		const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

		UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
		if (ClassPin && ClassPin->DefaultObject != nullptr && ClassPin->LinkedTo.Num() == 0)
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

UBlueprint* UK2Node_Action::GetActionBlueprint() const
{
	UClass* ClassToSpawn = GetActionClass();

	if (!ClassToSpawn)
		return nullptr;

	FString ClassPath = ClassToSpawn->GetPathName();
	ClassPath.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	const FString BPPath = FString::Printf(TEXT("Blueprint'%s'"), *ClassPath);

	return Cast<UBlueprint>(
		StaticFindObject(UBlueprint::StaticClass(), nullptr, *BPPath)
	);
}

bool UK2Node_Action::UseWorldContext() const
{
	auto BP = GetBlueprint();
	const UClass* ParentClass = BP ? BP->ParentClass : nullptr;
	return ParentClass ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

bool UK2Node_Action::HasWorldContext() const
{
	return FBlueprintEditorUtils::ImplementsGetWorld(GetBlueprint());
}

FText UK2Node_Action::GetBaseNodeTitle() const
{
	return LOCTEXT("Action_BaseTitle", "Action");
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
		if (IsActionVarPin(OldPin))
		{
			Pins.Remove(OldPin);
			OldClassPins.Add(OldPin);
		}
	}

	CachedNodeTitle.MarkDirty();

	TArray<UEdGraphPin*> NewClassPins;
	if (UClass* UseSpawnClass = GetActionClass())
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
	RewireOldPinsToNewPins(OldClassPins, Pins, nullptr);

	// Refresh the UI for the graph so the pin changes show up
	GetGraph()->NotifyGraphChanged();

	// Mark dirty
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_Action::OnBlueprintCompiled(UBlueprint* CompiledBP)
{
	// Ignore the blueprint if its a circular action dependency
	if (GetBlueprint() != CompiledBP && ActionBlueprint == CompiledBP)
	{
		OnClassPinChanged();
	}
}

void UK2Node_Action::BindBlueprintCompile()
{
	if (ActionBlueprint)
	{
		ActionBlueprint->OnCompiled().RemoveAll(this);
		ActionBlueprint = nullptr;
	}

	if (UBlueprint* BPToSpawn = GetActionBlueprint())
	{
		BPToSpawn->OnCompiled().AddUObject(this, &UK2Node_Action::OnBlueprintCompiled);
		ActionBlueprint = BPToSpawn;
	}
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
			bResult &= (nullptr != CENode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output));
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