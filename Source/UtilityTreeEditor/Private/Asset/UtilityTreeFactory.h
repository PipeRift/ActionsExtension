// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "UtilityTreeFactory.generated.h"

UCLASS()
class UUtilityTreeFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

	// The type of blueprint that will be created
	UPROPERTY(EditAnywhere, Category = WidgetBlueprintFactory)
	TEnumAsByte<enum EBlueprintType> BlueprintType;

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category = WidgetBlueprintFactory, meta = (AllowAbstract = ""))
	TSubclassOf<class UUserWidget> ParentClass;

    // UFactory interface
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    // End of UFactory interface
};
