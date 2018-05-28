// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraph.h"
#include "UtilityTreeGraph.generated.h"

class UEdGraphPin;

/** Delegate fired when a pin's default value is changed */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPinDefaultValueChanged, UEdGraphPin* /*InPinThatChanged*/)

UCLASS()
class UUtilityTreeGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	/** Delegate fired when a pin's default value is changed */
	FOnPinDefaultValueChanged OnPinDefaultValueChanged;
};

