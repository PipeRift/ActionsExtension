// Copyright 2015-2016 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Faction.h"
#include "FactionAgentInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFactionAgentInterface : public UGenericTeamAgentInterface
{
    GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class AIEXTENSION_API IFactionAgentInterface : public IGenericTeamAgentInterface
{
    GENERATED_IINTERFACE_BODY()


public:

    /** Retrieve faction identifier in form of Faction */
    UFUNCTION(BlueprintImplementableEvent, Category = Faction)
    FFaction GetFaction() const;

    /** Assigns faction */
    UFUNCTION(BlueprintImplementableEvent, Category = Faction)
    void SetFaction(const FFaction& Factory) const;

private:
    /** Begin GenericTeamAgent interface */

    /** Assigns Team Agent to given TeamID */
    virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override {
        SetFaction(FFaction(TeamID));
    }

    /** Retrieve team identifier in form of FGenericTeamId */
    virtual FGenericTeamId GetGenericTeamId() const override {
        return GetFaction().Team;
    }

    /** Retrieved owner attitude toward given Other object */
    virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override
    {
        const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
        return OtherTeamAgent ? FGenericTeamId::GetAttitude(GetGenericTeamId(), OtherTeamAgent->GetGenericTeamId())
            : ETeamAttitude::Neutral;
    }

    /** End GenericTeamAgent interface */
};
