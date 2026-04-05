// Copyright 2015-2026 Piperift. All Rights Reserved.

#pragma once

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"


class UK2Node_Action;

struct FActionNodeHelpers
{
	static void RegisterActionClassActions(
		FBlueprintActionDatabaseRegistrar& InActionRegister, UClass* NodeClass);

	static void SetNodeFunc(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UClass> ClassPtr);


	static int32 RegistryActionClassAction(
		FBlueprintActionDatabaseRegistrar& InActionRegistar, UClass* NodeClass, UClass* Class);

	template <typename TBase>
	static void GetAllBlueprintSubclasses(
		TSet<TSoftClassPtr<TBase>>& OutSubclasses, bool bAllowAbstract, FString const& Path);

	// Modified FKismetCompilerUtilities::GenerateAssignmentNodes
	static UEdGraphPin* GenerateAssignmentNodes(class FKismetCompilerContext& CompilerContext,
		UEdGraph* SourceGraph, UEdGraphPin* LastThenPin, UK2Node* CallBeginSpawnNode, UEdGraphNode* SpawnNode,
		UEdGraphPin* CallBeginResult, const UClass* ForClass,
		const UEdGraphPin* CallBeginClassInput = nullptr);
};