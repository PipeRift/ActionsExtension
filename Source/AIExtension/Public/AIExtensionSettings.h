// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Object.h"
#include "AIExtensionSettings.generated.h"

/**
 * Find Custom Config documentation here: wiki.unrealengine.com/CustomSettings 
 */
UCLASS(config = Game, defaultconfig)
class AIEXTENSION_API UAIExtensionSettings : public UObject
{
    GENERATED_BODY()
    
public:
    UAIExtensionSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};
