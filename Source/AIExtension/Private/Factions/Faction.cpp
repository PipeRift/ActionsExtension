// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIExtensionSettings.h"

#include "FactionInfo.h"

#include "Faction.h"

const FFaction FFaction::NoFaction(NO_FACTION);

FFactionInfo* FFaction::GetFactionInfo() const
{
    UAIExtensionSettings* Settings = GetMutableDefault<UAIExtensionSettings>();

    if (Settings->Factions.IsValidIndex(Id)) {
        return &Settings->Factions[Id];
    }

    //If the faction is not found, return default faction info.
    return nullptr;
}
