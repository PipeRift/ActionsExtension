// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "UtilityTreeBlueprintFactory.h"

#include "KismetEditorUtilities.h"
#include "BlueprintEditorUtils.h"

#include "UtilityTreeBlueprint.h"
#include "UtilityTree/UtilityTreeGraph.h"
#include "UtilityTree/UtilityTreeGraphSchema.h"

#define LOCTEXT_NAMESPACE "UtilityTree"


UUtilityTreeBlueprintFactory::UUtilityTreeBlueprintFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UUtilityTree::StaticClass();

	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UUtilityTreeBlueprint::StaticClass();
	ParentClass = UUtilityTree::StaticClass();
}

UObject* UUtilityTreeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a Anim Blueprint, then create and init one
	check(Class->IsChildOf(UUtilityTreeBlueprint::StaticClass()));

	// If they selected an interface, force the parent class to be UInterface
	if (BlueprintType == BPTYPE_Interface)
	{
		ParentClass = UInterface::StaticClass();
	}

	if ((ParentClass == NULL) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(UUtilityTree::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != NULL) ? FText::FromString(ParentClass->GetName()) : LOCTEXT("Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotCreateWidgetBlueprint", "Cannot create a Utility Tree Blueprint based on the class '{ClassName}'."), Args));
		return NULL;
	}
	else
	{
		UUtilityTreeBlueprint* NewBP = CastChecked<UUtilityTreeBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BlueprintType, UUtilityTreeBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CallingContext));
		
		UUtilityTreeBlueprint* RootUTBP = UUtilityTreeBlueprint::FindRootUtilityTreeBlueprint(NewBP);
		if (RootUTBP == NULL)
		{
			// Only allow an utility tree graph if there isn't one in a parent blueprint
			UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(NewBP, TEXT("Utility Graph"), UUtilityTreeGraph::StaticClass(), UUtilityTreeGraphSchema::StaticClass());
			FBlueprintEditorUtils::AddDomainSpecificGraph(NewBP, NewGraph);
			NewBP->LastEditedDocuments.Add(NewGraph);
			NewGraph->bAllowDeletion = false;
		}

		return NewBP;
	}

	UUtilityTree* UtilityTree = NewObject<UUtilityTree>(InParent, UUtilityTree::StaticClass(), Name, Flags);

	return UtilityTree;
}

UObject* UUtilityTreeBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

#undef LOCTEXT_NAMESPACE
