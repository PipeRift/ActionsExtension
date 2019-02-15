// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "Object.h"

#include "ActionsSettings.generated.h"


/**
 * Find Custom Config documentation here: wiki.unrealengine.com/CustomSettings 
 */
UCLASS(config = Game, defaultconfig)
class ACTIONS_API UActionsSettings : public UObject
{
	GENERATED_BODY()

public:

	UActionsSettings() : Super() {}
};
