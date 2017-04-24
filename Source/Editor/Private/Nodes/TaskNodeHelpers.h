// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
//#include "Kismet2/BlueprintEditorUtils.h"

#include "Task.h"

class UK2Node_Task;

struct TaskNodeHelpers {
    static void RegisterTaskClassActions(FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass);

    static void SetNodeFunc(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UClass> ClassPtr);


    static int32 RegistryTaskClassAction(FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass, UClass* Class);

    template < typename TBase >
    static void GetAllBlueprintSubclasses(TSet< TAssetSubclassOf< TBase > >& OutSubclasses, bool bAllowAbstract, FString const& Path);
};