// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "UtilityTreeBlueprintFactory.generated.h"

UCLASS()
class UUtilityTreeBlueprintFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

	// The type of blueprint that will be created
	UPROPERTY(EditAnywhere, Category = UtilityTreeBlueprintFactory)
	TEnumAsByte<enum EBlueprintType> BlueprintType;

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category = UtilityTreeBlueprintFactory, meta = (AllowAbstract = ""))
	TSubclassOf<class UUtilityTree> ParentClass;

    // UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    // End of UFactory interface
};
