// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIExtensionSettings.h"

#include "FactionInfo.h"

#include "Faction.h"

bool FFaction::GetFactionInfo(FFactionInfo& Info)
{
    const UAIExtensionSettings* Settings = GetDefault<UAIExtensionSettings>();

    if (Settings->Factions.IsValidIndex(Id)) {
        Info = Settings->Factions[Id];
        return true;
    }

    //If the faction is not found, return default faction info.
    Info = FFactionInfo();
    return false;
}
