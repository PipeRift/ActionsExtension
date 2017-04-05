// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AIExtensionEditorPrivatePCH.h"

#include "K2Node_ConstructAsyncObjectFromClass.h"
#include "UObject/UnrealType.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"

#include "TaskOwnerInterface.h"
#include "K2Node_AssignmentStatement.h"

FString UK2Node_ConstructAsyncObjectFromClass::FHelper::WorldContextPinName(TEXT("WorldContextObject"));
FString UK2Node_ConstructAsyncObjectFromClass::FHelper::ClassPinName(TEXT("Class"));
FString UK2Node_ConstructAsyncObjectFromClass::FHelper::OwnerPinName(TEXT("Owner"));

#define LOCTEXT_NAMESPACE "K2Node_ConstructAsyncObjectFromClass"

UK2Node_ConstructAsyncObjectFromClass::UK2Node_ConstructAsyncObjectFromClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to spawn a new object");
}

UClass* UK2Node_ConstructAsyncObjectFromClass::GetClassPinBaseClass() const
{
	return UObject::StaticClass();
}

bool UK2Node_ConstructAsyncObjectFromClass::UseWorldContext() const
{
	auto BP = GetBlueprint();
	const UClass* ParentClass = BP ? BP->ParentClass : nullptr;
	return ParentClass ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

void UK2Node_ConstructAsyncObjectFromClass::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	// Task Owner
	CreatePin(EGPD_Input, K2Schema->PC_Interface, TEXT(""), UTaskOwnerInterface::StaticClass(), false, false, FHelper::OwnerPinName);

	// Add blueprint pin
	UEdGraphPin* ClassPin = CreatePin(EGPD_Input, K2Schema->PC_Class, TEXT(""), GetClassPinBaseClass(), false, false, FHelper::ClassPinName);
	
	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), GetClassPinBaseClass(), false, false, K2Schema->PN_ReturnValue);

	Super::AllocateDefaultPins();
}

UEdGraphPin* UK2Node_ConstructAsyncObjectFromClass::GetOwnerPin() const
{
	UEdGraphPin* Pin = FindPin(FHelper::OwnerPinName);
	ensure(nullptr == Pin || Pin->Direction == EGPD_Input);
	return Pin;
}

void UK2Node_ConstructAsyncObjectFromClass::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
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

void UK2Node_ConstructAsyncObjectFromClass::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
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
		    if(	bIsExposedToSpawn &&
			    !Property->HasAnyPropertyFlags(CPF_Parm) && 
			    bIsSettableExternally &&
			    Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			    !bIsDelegate)
		    {
			    UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Property->GetName());
			    const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Property, /*out*/ Pin->PinType);
			
			    if (OutClassPins && Pin)
			    {
				    OutClassPins->Add(Pin);
			    }

			    if (ClassDefaultObject && Pin != NULL && K2Schema->PinDefaultValueIsEditable(*Pin))
			    {
				    FString DefaultValueAsString;
				    const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString);
				    check( bDefaultValueSet );
				    K2Schema->TrySetDefaultValue(*Pin, DefaultValueAsString);
			    }

			    // Copy tooltip from the property.
			    if (Pin != nullptr)
			    {
				    K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
			    }
		    }
            else if (bIsDelegate) {
                if (UMulticastDelegateProperty* Delegate = Cast<UMulticastDelegateProperty>(*PropertyIt))
                {
                    UFunction* DelegateSignatureFunction = Delegate->SignatureFunction;
                    UEdGraphPin* Pin;
                    if (DelegateSignatureFunction->NumParms < 1) {
                        Pin = CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, Delegate->GetName());
                    }
                    else {
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

UClass* UK2Node_ConstructAsyncObjectFromClass::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
	UClass* UseSpawnClass = NULL;
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
	if(ClassPin && ClassPin->DefaultObject != NULL && ClassPin->LinkedTo.Num() == 0)
	{
		UseSpawnClass = CastChecked<UClass>(ClassPin->DefaultObject);
	}
	else if (ClassPin && ClassPin->LinkedTo.Num())
	{
		auto ClassSource = ClassPin->LinkedTo[0];
		UseSpawnClass = ClassSource ? Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get()) : nullptr;
	}

	return UseSpawnClass;
}

void UK2Node_ConstructAsyncObjectFromClass::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) 
{
	AllocateDefaultPins();
	UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
	if( UseSpawnClass != NULL )
	{
		CreatePinsForClass(UseSpawnClass);
	}
	RestoreSplitPins(OldPins);
}

bool UK2Node_ConstructAsyncObjectFromClass::IsSpawnVarPin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	return(	Pin->PinName != K2Schema->PN_Execute &&
			Pin->PinName != K2Schema->PN_Then &&
			Pin->PinName != K2Schema->PN_ReturnValue &&
			Pin->PinName != FHelper::ClassPinName &&
			Pin->PinName != FHelper::WorldContextPinName &&
			Pin->PinName != FHelper::OwnerPinName);
}

void UK2Node_ConstructAsyncObjectFromClass::OnClassPinChanged()
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

void UK2Node_ConstructAsyncObjectFromClass::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin && (Pin->PinName == FHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

void UK2Node_ConstructAsyncObjectFromClass::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
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

void UK2Node_ConstructAsyncObjectFromClass::PinDefaultValueChanged(UEdGraphPin* ChangedPin) 
{
	if (ChangedPin && (ChangedPin->PinName == FHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

FText UK2Node_ConstructAsyncObjectFromClass::GetTooltipText() const
{
	return NodeTooltip;
}

UEdGraphPin* UK2Node_ConstructAsyncObjectFromClass::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_ConstructAsyncObjectFromClass::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = NULL;
	for( auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt )
	{
		UEdGraphPin* TestPin = *PinIt;
		if( TestPin && TestPin->PinName == FHelper::ClassPinName )
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_ConstructAsyncObjectFromClass::GetWorldContextPin(bool bChecked) const
{
	UEdGraphPin* Pin = FindPin(FHelper::WorldContextPinName);
    if (bChecked) {
        check(Pin == NULL || Pin->Direction == EGPD_Input);
    }
	return Pin;
}

UEdGraphPin* UK2Node_ConstructAsyncObjectFromClass::GetResultPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

FLinearColor UK2Node_ConstructAsyncObjectFromClass::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

FText UK2Node_ConstructAsyncObjectFromClass::GetBaseNodeTitle() const
{
	return NSLOCTEXT("K2Node", "ConstructObject_BaseTitle", "Construct Object from Class");
}

FText UK2Node_ConstructAsyncObjectFromClass::GetNodeTitleFormat() const
{
	return NSLOCTEXT("K2Node", "Construct", "Construct {ClassName}");
}

FText UK2Node_ConstructAsyncObjectFromClass::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
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
	return NSLOCTEXT("K2Node", "ConstructObject_Title_NONE", "Construct NONE");
}

bool UK2Node_ConstructAsyncObjectFromClass::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const 
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

void UK2Node_ConstructAsyncObjectFromClass::GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const
{
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT( "InvalidClass" );
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "ConstructAsyncObjectFromClass" ) ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "ObjectClass" ), ClassToSpawnStr ));
}

void UK2Node_ConstructAsyncObjectFromClass::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_ConstructAsyncObjectFromClass::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Gameplay);
}

bool UK2Node_ConstructAsyncObjectFromClass::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
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


bool UK2Node_ConstructAsyncObjectFromClass::FHelper::ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction, const UEdGraphSchema_K2* Schema)
{
    check(Schema);
    const bool bValidDataPin = Pin
        && (Pin->PinName != Schema->PN_Execute)
        && (Pin->PinName != Schema->PN_Then)
        && (Pin->PinType.PinCategory != Schema->PC_Exec);

    const bool bProperDirection = Pin && (Pin->Direction == Direction);

    return bValidDataPin && bProperDirection;
}

bool UK2Node_ConstructAsyncObjectFromClass::FHelper::CreateDelegateForNewFunction(UEdGraphPin* DelegateInputPin, FName FunctionName, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
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

bool UK2Node_ConstructAsyncObjectFromClass::FHelper::CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
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

bool UK2Node_ConstructAsyncObjectFromClass::FHelper::HandleDelegateImplementation(
    UMulticastDelegateProperty* CurrentProperty, const TArray<FHelper::FOutputPinAndLocalVariable>& VariableOutputs,
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
        AddDelegateNode->SetFromProperty(CurrentProperty, false);
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
    for (auto OutputPair : VariableOutputs) // CREATE CHAIN OF ASSIGMENTS
    {
        UEdGraphPin* PinWithData = CurrentCENode->FindPin(OutputPair.OutputPin->PinName);
        if (PinWithData == NULL)
        {
            /*FText ErrorMessage = FText::Format(LOCTEXT("MissingDataPin", "ICE: Pin @@ was expecting a data output pin named {0} on @@ (each delegate must have the same signature)"), FText::FromString(OutputPair.OutputPin->PinName));
            CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), OutputPair.OutputPin, CurrentCENode);
            return false;*/
            continue;
        }

        UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(CurrentNode, SourceGraph);
        AssignNode->AllocateDefaultPins();
        bIsErrorFree &= Schema->TryCreateConnection(LastActivatedNodeThen, AssignNode->GetExecPin());
        bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
        AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
        bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), PinWithData);
        AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

        LastActivatedNodeThen = AssignNode->GetThenPin();
    }

    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PinForCurrentDelegateProperty, *LastActivatedNodeThen).CanSafeConnect();
    return bIsErrorFree;
}


bool UK2Node_ConstructAsyncObjectFromClass::FHelper::HandleDelegateBindImplementation(
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
