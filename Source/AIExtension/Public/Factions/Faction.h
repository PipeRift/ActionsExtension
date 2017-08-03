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

    FFaction() {}

    FFaction(int32 InId) : Id(InId) {}

    FFaction(const FGenericTeamId& InTeam)
    {
        if (InTeam.GetId() == FGenericTeamId::NoTeam.GetId())
            Id = NO_FACTION;
        else
            Id = InTeam.GetId();
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Id;

    FFactionInfo* GetFactionInfo() const;

    FORCEINLINE const FGenericTeamId GetTeam() const {
        return Id != NO_FACTION? FGenericTeamId(Id) : FGenericTeamId::NoTeam;
    }

    FORCEINLINE bool IsNone() const {
        return Id == NO_FACTION;
    }

    /**
     * Attitude evaluation
     */
    FORCEINLINE bool IsHostileTo(const FFaction& Other) const {
        return AttitudeTowards(Other) == ETeamAttitude::Hostile;
    }

    const ETeamAttitude::Type AttitudeTowards(const FFaction& Other) const {
        if (this->IsNone() || Other.IsNone()) {
            return ETeamAttitude::Hostile;
        }

        const UAIExtensionSettings* Settings = GetDefault<UAIExtensionSettings>();

        const FFactionRelation* FoundRelationPtr = Settings->Relations.Find(FFactionRelation(*this, Other));
        if (FoundRelationPtr == NULL)
            return ETeamAttitude::Neutral;

        return FoundRelationPtr->Attitude;
    }

    /**
     * Operator overloading & Hashes
     */
    FORCEINLINE bool operator==(const FFaction& Other) const { return Id == Other.Id; }
    FORCEINLINE bool operator!=(const FFaction& Other) const { return !(*this == Other); }

    // Implicit conversion to GenericTeamId
    operator FGenericTeamId() const
    {
        return GetTeam();
    }

    friend uint32 GetTypeHash(const FFaction& InRelation)
    {
        return InRelation.IsNone() ? BIG_NUMBER : (uint32)InRelation.Id;
    }

    static const FFaction NoFaction;
};
