// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "GenericTeamAgentInterface.h"

#include "Faction.generated.h"

#define NO_FACTION -1
#define NO_FACTION_NAME FString("None")

struct FFactionInfo;

/**
 * 
 */
USTRUCT(BlueprintType)
struct AIEXTENSION_API FFaction
{
    GENERATED_USTRUCT_BODY()


    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Id;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGenericTeamId Team;

    bool GetFactionInfo(FFactionInfo& Info);


    FORCEINLINE bool operator==(const FFaction& Other) const { return Team == Other.Team; }
    FORCEINLINE bool operator!=(const FFaction& Other) const { return !(*this == Other); }

    bool IsHostileTo(FFaction& Other) const {
        //If are different factions or are None it's hostile
        return *this != Other || Other.Team == FGenericTeamId::NoTeam;
    }
};
