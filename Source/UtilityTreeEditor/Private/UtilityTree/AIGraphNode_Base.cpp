// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "UtilityTree/AIGraphNode_Base.h"
#include "UtilityTree/UtilityTreeGraph.h"
#include "UtilityTree/UtilityTreeGraphSchema.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "UtilityTree.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "UtilityTree/UTBlueprintNodeOptionalPinManager.h"
#include "EditorModeManager.h"

/////////////////////////////////////////////////////
// UUTGraphNode_Base

UAIGraphNode_Base::UAIGraphNode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAIGraphNode_Base::PreEditChange(UProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if (PropertyThatWillChange && PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(FOptionalPinFromProperty, bShowPin))
	{
		FOptionalPinManager::CacheShownPins(ShowPinForProperties, OldShownPins);
	}
}

void UAIGraphNode_Base::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(FOptionalPinFromProperty, bShowPin)))
	{
		FOptionalPinManager::EvaluateOldShownPins(ShowPinForProperties, OldShownPins, this);
		GetSchema()->ReconstructNode(*this);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	PropertyChangeEvent.Broadcast(PropertyChangedEvent);
}

void UAIGraphNode_Base::CreateOutputPins()
{
	if (!IsSinkNode())
	{
		const UUtilityTreeGraphSchema* Schema = GetDefault<UUtilityTreeGraphSchema>();
		CreatePin(EGPD_Output, Schema->PC_Struct, FAILink::StaticStruct(), { "Pose" });
	}
}

void UAIGraphNode_Base::InternalPinCreation(TArray<UEdGraphPin*>* OldPins)
{
	// pre-load required assets first before creating pins
	PreloadRequiredAssets();

	const UUtilityTreeGraphSchema* Schema = GetDefault<UUtilityTreeGraphSchema>();
	if (const UStructProperty* NodeStruct = GetFNodeProperty())
	{
		// Display any currently visible optional pins
		{
			UObject* NodeDefaults = GetArchetype();
			FUTBlueprintNodeOptionalPinManager OptionalPinManager(this, OldPins);
			OptionalPinManager.AllocateDefaultPins(NodeStruct->Struct, NodeStruct->ContainerPtrToValuePtr<uint8>(this), NodeDefaults ? NodeStruct->ContainerPtrToValuePtr<uint8>(NodeDefaults) : nullptr);
		}

		// Create the output pin, if needed
		CreateOutputPins();
	}
}

void UAIGraphNode_Base::AllocateDefaultPins()
{
	InternalPinCreation(NULL);
}

void UAIGraphNode_Base::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	InternalPinCreation(&OldPins);

	RestoreSplitPins(OldPins);
}

FLinearColor UAIGraphNode_Base::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

UScriptStruct* UAIGraphNode_Base::GetFNodeType() const
{
	UScriptStruct* BaseFStruct = FAINode_Base::StaticStruct();

	for (TFieldIterator<UProperty> PropIt(GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		if (UStructProperty* StructProp = Cast<UStructProperty>(*PropIt))
		{
			if (StructProp->Struct->IsChildOf(BaseFStruct))
			{
				return StructProp->Struct;
			}
		}
	}

	return NULL;
}

UStructProperty* UAIGraphNode_Base::GetFNodeProperty() const
{
	UScriptStruct* BaseFStruct = FAINode_Base::StaticStruct();

	for (TFieldIterator<UProperty> PropIt(GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		if (UStructProperty* StructProp = Cast<UStructProperty>(*PropIt))
		{
			if (StructProp->Struct->IsChildOf(BaseFStruct))
			{
				return StructProp;
			}
		}
	}

	return NULL;
}

FString UAIGraphNode_Base::GetNodeCategory() const
{
	return TEXT("Misc.");
}

void UAIGraphNode_Base::GetNodeAttributes( TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes ) const
{
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "UTGraphNode" ) ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), GetName() ));
}

void UAIGraphNode_Base::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

FText UAIGraphNode_Base::GetMenuCategory() const
{
	return FText::FromString(GetNodeCategory());
}

void UAIGraphNode_Base::GetPinAssociatedProperty(const UScriptStruct* NodeType, const UEdGraphPin* InputPin, UProperty*& OutProperty, int32& OutIndex) const
{
	OutProperty = nullptr;
	OutIndex = INDEX_NONE;

	//@TODO: Name-based hackery, avoid the roundtrip and better indicate when it's an array pose pin
	int32 UnderscoreIndex = InputPin->PinName.ToString().Find(TEXT("_"), ESearchCase::CaseSensitive);
	if (UnderscoreIndex != INDEX_NONE)
	{
		FString ArrayName = InputPin->PinName.ToString().Left(UnderscoreIndex);
		int32 ArrayIndex = FCString::Atoi(*(InputPin->PinName.ToString().Mid(UnderscoreIndex + 1)));

		if (UArrayProperty* ArrayProperty = FindField<UArrayProperty>(NodeType, *ArrayName))
		{
			OutProperty = ArrayProperty;
			OutIndex = ArrayIndex;
		}
	}

	// If the array check failed or we have no underscores
	if(OutProperty == nullptr)
	{
		if (UProperty* Property = FindField<UProperty>(NodeType, *(InputPin->PinName.ToString())))
		{
			OutProperty = Property;
			OutIndex = INDEX_NONE;
		}
	}
}

FAILinkMappingRecord UAIGraphNode_Base::GetLinkIDLocation(const UScriptStruct* NodeType, UEdGraphPin* SourcePin)
{
	if (SourcePin->LinkedTo.Num() > 0)
	{
		if (UAIGraphNode_Base* LinkedNode = Cast<UAIGraphNode_Base>(FBlueprintEditorUtils::FindFirstCompilerRelevantNode(SourcePin->LinkedTo[0])))
		{
			//@TODO: Name-based hackery, avoid the roundtrip and better indicate when it's an array pose pin
			int32 UnderscoreIndex = SourcePin->PinName.ToString().Find(TEXT("_"), ESearchCase::CaseSensitive);
			if (UnderscoreIndex != INDEX_NONE)
			{
				FString ArrayName = SourcePin->PinName.ToString().Left(UnderscoreIndex);
				int32 ArrayIndex = FCString::Atoi(*(SourcePin->PinName.ToString().Mid(UnderscoreIndex + 1)));

				if (UArrayProperty* ArrayProperty = FindField<UArrayProperty>(NodeType, *ArrayName))
				{
					if (UStructProperty* Property = Cast<UStructProperty>(ArrayProperty->Inner))
					{
						if (Property->Struct->IsChildOf(FAILinkBase::StaticStruct()))
						{
							return FAILinkMappingRecord::MakeFromArrayEntry(this, LinkedNode, ArrayProperty, ArrayIndex);
						}
					}
				}
			}
			else
			{
				if (UStructProperty* Property = FindField<UStructProperty>(NodeType, *(SourcePin->PinName.ToString())))
				{
					if (Property->Struct->IsChildOf(FAILinkBase::StaticStruct()))
					{
						return FAILinkMappingRecord::MakeFromMember(this, LinkedNode, Property);
					}
				}
			}
		}
	}

	return FAILinkMappingRecord::MakeInvalid();
}

void UAIGraphNode_Base::CreatePinsForAILink(UProperty* AIProperty, int32 ArrayIndex)
{
	const UUtilityTreeGraphSchema* Schema = GetDefault<UUtilityTreeGraphSchema>();
	//UScriptStruct* A2PoseStruct = FA2Pose::StaticStruct();

	// pose input
	//const FString NewPinName = (ArrayIndex == INDEX_NONE) ? PoseProperty->GetName() : FString::Printf(TEXT("%s_%d"), *(PoseProperty->GetName()), ArrayIndex);
	//CreatePin(EGPD_Input, Schema->PC_Struct, FString(), A2PoseStruct, NewPinName);
}

void UAIGraphNode_Base::PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const
{
	if (Pin->Direction == EGPD_Output)
	{
		if (Pin->PinName == TEXT("Pose"))
		{
			DisplayName.Reset();
		}
	}
}

bool UAIGraphNode_Base::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	return DesiredSchema->GetClass()->IsChildOf(UUtilityTreeGraphSchema::StaticClass());
}

FString UAIGraphNode_Base::GetDocumentationLink() const
{
	return TEXT("");
}

void UAIGraphNode_Base::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	if (UUtilityTreeGraphSchema::IsAIPin(Pin.PinType))
	{
		HoverTextOut = TEXT("AI Pin");
	}
	else
	{
		Super::GetPinHoverText(Pin, HoverTextOut);
	}
}

void UAIGraphNode_Base::OnNodeSelected(bool bInIsSelected, FEditorModeTools& InModeTools, FAINode_Base* InRuntimeNode)
{
}

EUTAssetHandlerType UAIGraphNode_Base::SupportsAssetClass(const UClass* AssetClass) const
{
	return EUTAssetHandlerType::NotSupported;
}


void UAIGraphNode_Base::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	CopyPinDefaultsToNodeData(Pin);

	if(UUtilityTreeGraph* UTGraph = Cast<UUtilityTreeGraph>(GetGraph()))
	{
		UTGraph->OnPinDefaultValueChanged.Broadcast(Pin);
	}
}

bool UAIGraphNode_Base::IsPinExposedAndLinked(const FString& InPinName, const EEdGraphPinDirection InDirection) const
{
	UEdGraphPin* Pin = FindPin(InPinName, InDirection);
	return Pin != nullptr && Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] != nullptr;
}