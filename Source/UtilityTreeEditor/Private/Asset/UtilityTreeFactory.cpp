// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "UtilityTreeFactory.h"

#include "UtilityTree.h"

#define LOCTEXT_NAMESPACE "UtilityTree"


UUtilityTreeFactory::UUtilityTreeFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SupportedClass = UUtilityTree::StaticClass();

    bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UUtilityTreeBlueprint::StaticClass();
	ParentClass = UUtilityTree::StaticClass();
}

UObject* UUtilityTreeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
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

		return NewBP;
	}

    UUtilityTree* UtilityTree = NewObject<UUtilityTree>(InParent, UUtilityTree::StaticClass(), Name, Flags);

    return UtilityTree;
}

#undef LOCTEXT_NAMESPACE
