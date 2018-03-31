// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIExtensionSettings.h"

#include "FactionInfo.h"

#include "Faction.h"

const FFaction FFaction::NoFaction(NO_FACTION);

bool FFaction::GetFactionInfo(FFactionInfo& Info) const
{
    UAIExtensionSettings* Settings = GetMutableDefault<UAIExtensionSettings>();
	check(Settings);

    if (Settings->Factions.IsValidIndex(Id)) {
		Info = Settings->Factions[Id];
        return true;
    }

    //If the faction is not found, return default faction info.
    return false;
}

const ETeamAttitude::Type FFaction::GetAttitudeTowards(const FFaction& Other) const {
    const UAIExtensionSettings* Settings = GetDefault<UAIExtensionSettings>();

    const FFactionRelation* FoundRelationPtr = Settings->Relations.FindByKey(FFactionRelation(*this, Other));
    if (FoundRelationPtr == NULL) {
		//Relation not found, use default
		FFactionInfo Info;
        if (GetFactionInfo(Info))
        {
			if (*this == Other)
			{
				return Info.DefaultAttitudeToItself;
			}
			return Info.DefaultAttitudeToOthers;
        }
        return ETeamAttitude::Neutral;
    }

    return FoundRelationPtr->Attitude;
}
