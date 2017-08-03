// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "FactionInfo.h"

#include "AIExtensionSettings.h"


const FFaction FFactionInfo::GetFaction() 
{
    const auto* Self = this;
    //Find this faction by pointer
    const int32 Index = GetDefault<UAIExtensionSettings>()->Factions.IndexOfByPredicate([Self](const auto& FactionInfo) {
        return Self == &FactionInfo;
    });

    const FFaction Faction(Index);
    checkf(Faction.IsNone(), TEXT("Faction Info can never be None"))
    return Faction;
}

void FFactionInfo::SetRelation(const FFaction& OtherFaction, const ETeamAttitude::Type Attitude)
{
    if (OtherFaction.IsNone())
        return;

    FFactionRelation InRelation(this->GetFaction(), OtherFaction, Attitude);

    TSet<FFactionRelation>& Relations = GetMutableDefault<UAIExtensionSettings>()->Relations;

    //Remove possible similar relation
    FFactionRelation* const FoundRelationPtr = Relations.Find(InRelation);
    if (FoundRelationPtr == NULL) 
    {
        Relations.Add(InRelation);
    }
    else
    {
        FoundRelationPtr->Attitude = Attitude;
    }
}
