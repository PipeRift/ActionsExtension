// Copyright 2015-2026 Piperift. All Rights Reserved.

#include "ActionNodeHelpers.h"

#include "K2Node_Action.h"

#include <Action.h>
#include <AssetRegistry/ARFilter.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <BlueprintCompilationManager.h>
#include <K2Node_CallArrayFunction.h>
#include <K2Node_CallFunction.h>
#include <K2Node_DynamicCast.h>
#include <K2Node_EnumLiteral.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <KismetCompiler.h>
#include <KismetCompilerMisc.h>


void FActionNodeHelpers::RegisterActionClassActions(
	FBlueprintActionDatabaseRegistrar& InActionRegister, UClass* NodeClass)
{
	UClass* TaskType = UAction::StaticClass();

	int32 RegisteredCount = 0;
	if (const UObject* RegistrerTarget = InActionRegister.GetActionKeyFilter())
	{
		if (const auto* TargetClass = Cast<UClass>(RegistrerTarget))
		{
			if (!TargetClass->HasAnyClassFlags(CLASS_Abstract) && !TargetClass->IsChildOf(TaskType))
			{
				UBlueprintNodeSpawner* NewAction = UBlueprintNodeSpawner::Create(NodeClass);
				check(NewAction != nullptr);

				TWeakObjectPtr<UClass> TargetClassPtr = const_cast<UClass*>(TargetClass);
				NewAction->CustomizeNodeDelegate =
					UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(SetNodeFunc, TargetClassPtr);

				if (NewAction)
				{
					RegisteredCount += (int32) InActionRegister.AddBlueprintAction(TargetClass, NewAction);
				}
			}
		}
	}
	else
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* Class = *ClassIt;
			if (Class->HasAnyClassFlags(CLASS_Abstract) || !Class->IsChildOf(TaskType) || Class == TaskType)
			{
				continue;
			}

			RegisteredCount += RegistryActionClassAction(InActionRegister, NodeClass, Class);
		}

		// Registry blueprint classes
		/*TSet<TAssetSubclassOf<UAction>> BPClasses;
		GetAllBlueprintSubclasses<UAction>(BPClasses, false, "");
		for (auto& BPClass : BPClasses)
		{
			if (!BPClass.IsValid())
			{
				UClass* Class = BPClass.LoadSynchronous();
				if (Class)
				{
					RegisteredCount += RegistryActionClassAction(InActionRegister, NodeClass, Class);
				}
			}
		}*/
	}
	return;
}

void FActionNodeHelpers::SetNodeFunc(
	UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UClass> ClassPtr)
{
	UK2Node_Action* ActionNode = CastChecked<UK2Node_Action>(NewNode);
	if (ClassPtr.IsValid())
	{
		ActionNode->bShowClass = false;
		ActionNode->ActionClass = ClassPtr.Get();
	}
}

template <typename TBase>
void FActionNodeHelpers::GetAllBlueprintSubclasses(
	TSet<TSoftClassPtr<TBase>>& Subclasses, bool bAllowAbstract, FString const& Path)
{
	static const FName GeneratedClassTag = TEXT("GeneratedClass");
	static const FName ClassFlagsTag = TEXT("ClassFlags");

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Use the asset registry to get the set of all class names deriving from Base
	TSet<FTopLevelAssetPath> DerivedActorClassNames;
	AssetRegistry.GetDerivedClassNames(
		{TBase::StaticClass()->GetClassPathName()}, {}, DerivedActorClassNames);


	// Set up a filter and then pull asset data for all blueprints in the specified path from the asset
	// registry. Note that this works in packaged builds too. Even though the blueprint itself cannot be
	// loaded, its asset data still exists and is tied to the UBlueprint type.
	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	if (!Path.IsEmpty())
	{
		Filter.PackagePaths.Add(*Path);
	}
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	// Iterate over retrieved blueprint assets
	for (auto const& Asset : AssetList)
	{
		// Get the the class this blueprint generates (this is stored as a full path)
		auto GeneratedClassPath = Asset.TagsAndValues.FindTag(GeneratedClassTag);
		if (GeneratedClassPath.IsSet())
		{
			const FTopLevelAssetPath ClassPathName(
				FPackageName::ExportTextPathToObjectPath(*GeneratedClassPath.AsString()));

			// Check if this class is in the derived set
			if (!DerivedActorClassNames.Contains(ClassPathName))
			{
				continue;
			}

			// Store using the path to the generated class
			Subclasses.Add(TSoftClassPtr<TBase>(Asset.ToSoftObjectPath()));
		}
	}
}


int32 FActionNodeHelpers::RegistryActionClassAction(
	FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass, UClass* Class)
{
	UBlueprintNodeSpawner* NewAction = UBlueprintNodeSpawner::Create(NodeClass);
	check(NewAction != nullptr);

	NewAction->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
		SetNodeFunc, TWeakObjectPtr<UClass>{Class});

	if (NewAction)
	{
		return (int32) InActionRegistar.AddBlueprintAction(Class, NewAction);
	}

	return 0;
}


UEdGraphPin* FActionNodeHelpers::GenerateAssignmentNodes(FKismetCompilerContext& CompilerContext,
	UEdGraph* SourceGraph, UEdGraphPin* LastThenPin, UK2Node* CallBeginSpawnNode, UEdGraphNode* SpawnNode,
	UEdGraphPin* CallBeginResult, const UClass* ForClass, const UEdGraphPin* CallBeginClassInput)
{
	static const FName ObjectParamName(TEXT("Object"));
	static const FName ValueParamName(TEXT("Value"));
	static const FName PropertyNameParamName(TEXT("PropertyName"));

	int32 NumAssignments = 0;

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	if (!LastThenPin)
	{
		LastThenPin = CallBeginSpawnNode->GetThenPin();
	}

	// If the class input pin is linked, then 'ForClass' represents a base type, but the actual type might be
	// a derived class that won't get resolved until runtime. In that case, we can't use the base type's CDO
	// to avoid generating assignment statements for unlinked parameter inputs that match the base type's
	// default value for those parameters, since the derived type might store a different default value, which
	// would otherwise be used if we don't explicitly assign it.
	const bool bIsClassInputPinLinked = CallBeginClassInput && CallBeginClassInput->LinkedTo.Num() > 0;

	// Create 'set var by name' nodes and hook them up
	for (int32 PinIdx = 0; PinIdx < SpawnNode->Pins.Num(); PinIdx++)
	{
		// Only create 'set param by name' node if this pin is linked to something
		UEdGraphPin* OrgPin = SpawnNode->Pins[PinIdx];

		if (OrgPin->Direction == EGPD_Output)
		{
			continue;
		}

		if (!CallBeginSpawnNode->FindPin(OrgPin->PinName))
		{
			FProperty* Property = FindFProperty<FProperty>(ForClass, OrgPin->PinName);
			// NULL property indicates that this pin was part of the original node, not the
			// class we're assigning to:
			if (!Property)
			{
				continue;
			}

			if (OrgPin->LinkedTo.Num() == 0)
			{
				FString DefaultValueErrorString = Schema->IsCurrentPinDefaultValid(OrgPin);
				if (!DefaultValueErrorString.IsEmpty())
				{
					// Some types require a connection for assignment (e.g. arrays).
					continue;
				}
				// If the property is not editable in blueprint, always check the native CDO to handle cases
				// like Instigator properly
				else if (!bIsClassInputPinLinked ||
						 Property->HasAnyPropertyFlags(CPF_DisableEditOnTemplate) ||
						 !Property->HasAnyPropertyFlags(CPF_Edit))
				{
					// We don't want to generate an assignment node unless the default value
					// differs from the value in the CDO:
					FString DefaultValueAsString;

					if (!FBlueprintCompilationManager::GetDefaultValue(
							ForClass, Property, DefaultValueAsString))
					{
						if (ForClass->GetDefaultObject(false))
						{
							FBlueprintEditorUtils::PropertyValueToString(
								Property, (uint8*) ForClass->GetDefaultObject(false), DefaultValueAsString);
						}
					}

					// First check the string representation of the default value
					if (Schema->DoesDefaultValueMatch(*OrgPin, DefaultValueAsString))
					{
						continue;
					}

					FString UseDefaultValue;
					TObjectPtr<UObject> UseDefaultObject = nullptr;
					FText UseDefaultText;
					constexpr bool bPreserveTextIdentity = true;

					// Next check if the converted default value would be the same to handle cases like None
					// for object pointers
					Schema->GetPinDefaultValuesFromString(OrgPin->PinType, OrgPin->GetOwningNodeUnchecked(),
						DefaultValueAsString, UseDefaultValue, UseDefaultObject, UseDefaultText,
						bPreserveTextIdentity);

					if (OrgPin->DefaultValue.Equals(UseDefaultValue, ESearchCase::CaseSensitive) &&
						OrgPin->DefaultObject == UseDefaultObject &&
						OrgPin->DefaultTextValue.IdenticalTo(UseDefaultText))
					{
						continue;
					}
				}
			}

			const FString& SetFunctionName =
				Property->GetMetaData(FBlueprintMetadata::MD_PropertySetFunction);
			if (!SetFunctionName.IsEmpty())
			{
				UFunction* SetFunction = ForClass->FindFunctionByName(*SetFunctionName);
				if (SetFunction == nullptr)
				{
					if (UBlueprint* Generator = Cast<UBlueprint>(ForClass->ClassGeneratedBy))
					{
						SetFunction = Generator->SkeletonGeneratedClass->FindFunctionByName(*SetFunctionName);
					}
				}
				checkf(SetFunction, TEXT("Failed to find SetFunction '%s' on class '%s'!"), *SetFunctionName,
					*ForClass->GetPathName());

				// Add a cast node so we can call the Setter function with a pin of the right class
				UK2Node_DynamicCast* CastNode =
					CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(SpawnNode, SourceGraph);
				CastNode->TargetType = const_cast<UClass*>(ForClass);
				CastNode->SetPurity(true);
				CastNode->AllocateDefaultPins();
				CastNode->GetCastSourcePin()->MakeLinkTo(CallBeginResult);
				CastNode->NotifyPinConnectionListChanged(CastNode->GetCastSourcePin());

				UK2Node_CallFunction* CallFuncNode =
					CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
				CallFuncNode->SetFromFunction(SetFunction);
				CallFuncNode->AllocateDefaultPins();

				// Connect this node into the exec chain
				Schema->TryCreateConnection(LastThenPin, CallFuncNode->GetExecPin());
				LastThenPin = CallFuncNode->GetThenPin();

				// Connect the new object to the 'object' pin
				UEdGraphPin* ObjectPin = Schema->FindSelfPin(*CallFuncNode, EGPD_Input);
				CastNode->GetCastResultPin()->MakeLinkTo(ObjectPin);

				// Move Value pin connections
				UEdGraphPin* SetFunctionValuePin = nullptr;
				for (UEdGraphPin* CallFuncPin : CallFuncNode->Pins)
				{
					if (!Schema->IsMetaPin(*CallFuncPin))
					{
						check(CallFuncPin->Direction == EGPD_Input);
						SetFunctionValuePin = CallFuncPin;
						break;
					}
				}
				check(SetFunctionValuePin);

				CompilerContext.MovePinLinksToIntermediate(*OrgPin, *SetFunctionValuePin);
			}
			else if (FKismetCompilerUtilities::IsPropertyUsesFieldNotificationSetValueAndBroadcast(Property))
			{
				// Add a cast node so we can call the Setter function with a pin of the right class
				// UK2Node_DynamicCast* CastNode =
				// CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(SpawnNode, SourceGraph);
				// CastNode->TargetType = const_cast<UClass*>(ForClass);
				// CastNode->SetPurity(true);
				// CastNode->AllocateDefaultPins();
				// CastNode->GetCastSourcePin()->MakeLinkTo(CallBeginResult);
				// CastNode->NotifyPinConnectionListChanged(CastNode->GetCastSourcePin());

				FMemberReference MemberReference;
				MemberReference.SetFromField<FProperty>(Property, false);
				TTuple<UEdGraphPin*, UEdGraphPin*> ExecThenPins =
					FKismetCompilerUtilities::GenerateFieldNotificationSetNode(CompilerContext, SourceGraph,
						SpawnNode, CallBeginResult, Property, MemberReference, false, false,
						Property->HasAllPropertyFlags(CPF_Net));

				// Connect this node into the exec chain
				Schema->TryCreateConnection(LastThenPin, ExecThenPins.Get<0>());
				LastThenPin = ExecThenPins.Get<1>();
			}
			else if (UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(OrgPin->PinType))
			{
				++NumAssignments;

				UK2Node_CallFunction* SetVarNode = nullptr;
				if (OrgPin->PinType.IsArray())
				{
					SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(
						SpawnNode, SourceGraph);
				}
				else
				{
					SetVarNode =
						CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
				}
				SetVarNode->SetFromFunction(SetByNameFunction);
				SetVarNode->AllocateDefaultPins();

				// Connect this node into the exec chain
				Schema->TryCreateConnection(LastThenPin, SetVarNode->GetExecPin());
				LastThenPin = SetVarNode->GetThenPin();

				// Connect the new object to the 'object' pin
				UEdGraphPin* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
				CallBeginResult->MakeLinkTo(ObjectPin);

				// Fill in literal for 'property name' pin - name of pin is property name
				UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
				PropertyNamePin->DefaultValue = OrgPin->PinName.ToString();

				UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
				if (OrgPin->LinkedTo.Num() == 0 && OrgPin->DefaultValue != FString() &&
					OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte &&
					OrgPin->PinType.PinSubCategoryObject.IsValid() &&
					OrgPin->PinType.PinSubCategoryObject->IsA<UEnum>())
				{
					// Pin is an enum, we need to alias the enum value to an int:
					UK2Node_EnumLiteral* EnumLiteralNode =
						CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(SpawnNode, SourceGraph);
					EnumLiteralNode->Enum = CastChecked<UEnum>(OrgPin->PinType.PinSubCategoryObject.Get());
					EnumLiteralNode->AllocateDefaultPins();
					EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ValuePin);

					UEdGraphPin* InPin =
						EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
					check(InPin);
					InPin->DefaultValue = OrgPin->DefaultValue;
				}
				else
				{
					// For non-array struct pins that are not linked, transfer the pin type so that the node
					// will expand an auto-ref that will assign the value by-ref.
					if (OrgPin->PinType.IsArray() == false &&
						OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
						OrgPin->LinkedTo.Num() == 0)
					{
						ValuePin->PinType.PinCategory = OrgPin->PinType.PinCategory;
						ValuePin->PinType.PinSubCategory = OrgPin->PinType.PinSubCategory;
						ValuePin->PinType.PinSubCategoryObject = OrgPin->PinType.PinSubCategoryObject;
						CompilerContext.MovePinLinksToIntermediate(*OrgPin, *ValuePin);
					}
					else
					{
						// For interface pins we need to copy over the subcategory
						if (OrgPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Interface)
						{
							ValuePin->PinType.PinSubCategoryObject = OrgPin->PinType.PinSubCategoryObject;
						}

						CompilerContext.MovePinLinksToIntermediate(*OrgPin, *ValuePin);
						SetVarNode->PinConnectionListChanged(ValuePin);
					}
				}
			}
		}
	}

	return LastThenPin;
}
