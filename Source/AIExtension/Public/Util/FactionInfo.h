// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "GenericTeamAgentInterface.h"

#include "Faction.h"
#include "FactionInfo.generated.h"


/**
 * 
 */
USTRUCT()
struct FFactionInfo
{
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Faction)
    FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Faction)
    FColor Color;

    FFactionInfo() : Color(FColor::Orange), Name(NO_FACTION_NAME)
    {}
};
