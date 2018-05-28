// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionEditorPrivatePCH.h"

#include "K2Node_Action.h"
#include "ARFilter.h"
#include "AssetRegistryModule.h"

#include "ActionNodeHelpers.h"

void ActionNodeHelpers::RegisterActionClassActions(FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass)
{
	UClass* TaskType = UAction::StaticClass();

	int32 RegisteredCount = 0;
	if (const UObject* RegistrarTarget = InActionRegistar.GetActionKeyFilter())
	{
		if (const UClass* TargetClass = Cast<UClass>(RegistrarTarget))
		{
			if (!TargetClass->HasAnyClassFlags(CLASS_Abstract) && !TargetClass->IsChildOf(TaskType))
			{

				UBlueprintNodeSpawner* NewAction = UBlueprintNodeSpawner::Create(NodeClass);
				check(NewAction != nullptr);

				TWeakObjectPtr<UClass> TargetClassPtr = TargetClass;
				NewAction->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(SetNodeFunc, TargetClassPtr);

				if (NewAction)
				{
					RegisteredCount += (int32)InActionRegistar.AddBlueprintAction(TargetClass, NewAction);
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

			RegisteredCount += RegistryActionClassAction(InActionRegistar, NodeClass, Class);
		}

		//Registry blueprint classes
		TSet<TAssetSubclassOf<UAction>> BPClasses;
		GetAllBlueprintSubclasses<UAction>(BPClasses, false, "");
		for (auto& BPClass : BPClasses)
		{
			if (!BPClass.IsNull())
			{
				UClass* Class = BPClass.LoadSynchronous();
				if (Class)
				{
					RegisteredCount += RegistryActionClassAction(InActionRegistar, NodeClass, Class);
				}
			}
		}
	}
	return;
}

void ActionNodeHelpers::SetNodeFunc(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UClass> ClassPtr)
{
	UK2Node_Action* TaskNode = CastChecked<UK2Node_Action>(NewNode);
	if (ClassPtr.IsValid())
	{
		TaskNode->PrestatedClass = ClassPtr.Get();
	}
}

template<typename TBase>
void ActionNodeHelpers::GetAllBlueprintSubclasses(TSet< TAssetSubclassOf< TBase > >& Subclasses, bool bAllowAbstract, FString const& Path)
{
	static const FName GeneratedClassTag = TEXT("GeneratedClass");
	static const FName ClassFlagsTag = TEXT("ClassFlags");

	UClass* Base = TBase::StaticClass();
	check(Base);

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FName BaseClassName = Base->GetFName();

	// Use the asset registry to get the set of all class names deriving from Base
	TSet< FName > DerivedNames;
	{
		TArray< FName > BaseNames;
		BaseNames.Add(BaseClassName);

		TSet< FName > Excluded;
		AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);
	}

	// Set up a filter and then pull asset data for all blueprints in the specified path from the asset registry.
	// Note that this works in packaged builds too. Even though the blueprint itself cannot be loaded, its asset data
	// still exists and is tied to the UBlueprint type.
	FARFilter Filter;
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;
	if (!Path.IsEmpty())
	{
		Filter.PackagePaths.Add(*Path);
	}
	Filter.bRecursivePaths = true;

	TArray< FAssetData > AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	// Iterate over retrieved blueprint assets
	for (auto const& Asset : AssetList)
	{
		// Get the the class this blueprint generates (this is stored as a full path)
		if (auto GeneratedClassPathPtr = Asset.TagsAndValues.Find(GeneratedClassTag))
		{
			// Optionally ignore abstract classes
			// As of 4.12 I do not believe blueprints can be marked as abstract, but this may change so included for completeness.
			if (!bAllowAbstract)
			{
				if (auto ClassFlagsPtr = Asset.TagsAndValues.Find(ClassFlagsTag))
				{
					auto ClassFlags = FCString::Atoi(**ClassFlagsPtr);
					if ((ClassFlags & CLASS_Abstract) != 0)
					{
						continue;
					}
				}
			}

			// Convert path to just the name part
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr);
			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

			// Check if this class is in the derived set
			if (!DerivedNames.Contains(*ClassName))
			{
				continue;
			}

			// Store using the path to the generated class
			Subclasses.Add(TAssetSubclassOf< TBase >(FStringAssetReference(ClassObjectPath)));
		}
	}
}

int32 ActionNodeHelpers::RegistryActionClassAction(FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass, UClass* Class)
{
	UBlueprintNodeSpawner* NewAction = UBlueprintNodeSpawner::Create(NodeClass);
	check(NewAction != nullptr);

	TWeakObjectPtr<UClass> TargetClassPtr = Class;
	NewAction->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(SetNodeFunc, TargetClassPtr);

	if (NewAction)
	{
		return (int32)InActionRegistar.AddBlueprintAction(Class, NewAction);
	}

	return 0;
}
