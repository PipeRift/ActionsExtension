// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"

class UEdGraphPin;

struct FUTBlueprintNodeOptionalPinManager : public FOptionalPinManager
{
protected:
	class UAIGraphNode_Base* BaseNode;
	TArray<UEdGraphPin*>* OldPins;

	TMap<FString, UEdGraphPin*> OldPinMap;

public:
	FUTBlueprintNodeOptionalPinManager(class UAIGraphNode_Base* Node, TArray<UEdGraphPin*>* InOldPins);

	/** FOptionalPinManager interface */
	virtual void GetRecordDefaults(UProperty* TestProperty, FOptionalPinFromProperty& Record) const override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, UProperty* Property) const override;
	virtual void PostInitNewPin(UEdGraphPin* Pin, FOptionalPinFromProperty& Record, int32 ArrayIndex, UProperty* Property, uint8* PropertyAddress, uint8* DefaultPropertyAddress) const override;
	virtual void PostRemovedOldPin(FOptionalPinFromProperty& Record, int32 ArrayIndex, UProperty* Property, uint8* PropertyAddress, uint8* DefaultPropertyAddress) const override;

	void AllocateDefaultPins(UStruct* SourceStruct, uint8* StructBasePtr, uint8* DefaultsPtr);
};
