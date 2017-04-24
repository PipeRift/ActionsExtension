// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionEditorPrivatePCH.h"

#include "K2Node_Task.h"
#include "AssetRegistryModule.h"
#include "ARFilter.h"

#include "TaskNodeHelpers.h"

void TaskNodeHelpers::RegisterTaskClassActions(FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass)
{
    UClass* TaskType = UTask::StaticClass();

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
        // these nested loops are combing over the same classes/functions the
        // FBlueprintActionDatabase does; ideally we save on perf and fold this in
        // with FBlueprintActionDatabase, but we want to give separate modules
        // the opportunity to add their own actions per class func
        for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
        {
            UClass* Class = *ClassIt;
            if (Class->HasAnyClassFlags(CLASS_Abstract) || !Class->IsChildOf(TaskType))
            {
                continue;
            }

            RegisteredCount += RegistryTaskClassAction(InActionRegistar, NodeClass, Class);
        }

        //Registry blueprint classes
        TSet<UClass*> BPClasses = GetBlueprintTaskClasses();
        for (auto& BPClass : BPClasses) {
            RegisteredCount += RegistryTaskClassAction(InActionRegistar, NodeClass, BPClass);
        }
    }
    return;
}

void TaskNodeHelpers::SetNodeFunc(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UClass> ClassPtr)
{
    UK2Node_Task* TaskNode = CastChecked<UK2Node_Task>(NewNode);
    if (ClassPtr.IsValid())
    {
        TaskNode->PrestatedClass = ClassPtr.Get();
    }
}


TSet<UClass*> TaskNodeHelpers::GetBlueprintTaskClasses() {
    // Load the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FName TaskClassName = UTask::StaticClass()->GetFName();

    TSet< FName > DerivedClassNames;
    {
        TArray< FName > BaseNames;
        BaseNames.Add(TaskClassName);

        TSet< FName > Excluded;
        AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedClassNames);
    }

    FARFilter Filter;
    Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;

    TArray< FAssetData > AssetList;
    AssetRegistry.GetAssets(Filter, AssetList);

    TSet<UClass*> Subclasses;

    // Iterate over retrieved blueprint assets
    for (auto const& Asset : AssetList)
    {
        // Get the the class this blueprint generates (this is stored as a full path)
        if (auto GeneratedClassPathPtr = Asset.TagsAndValues.Find(TEXT("GeneratedClass")))
        {
            // Convert path to just the name part
            const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr);
            const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

            // Check if this class is in the derived set
            if (!DerivedClassNames.Contains(*ClassName))
            {
                continue;
            }

            // Store using the path to the generated class
            if(UClass* Subclass = TAssetSubclassOf< UTask >(FStringClassReference(ClassObjectPath)).Get())
            {
                Subclasses.Add(Subclass);
            }
        }
    }

    return Subclasses;
}

int32 TaskNodeHelpers::RegistryTaskClassAction(FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass, UClass* Class) {
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
