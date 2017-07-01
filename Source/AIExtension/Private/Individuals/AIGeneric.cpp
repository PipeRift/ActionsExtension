// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "AIGeneric.h"

AAIGeneric::AAIGeneric(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    
    State = ECombatState::Passive;
}

void AAIGeneric::JoinSquad(AAISquad * SquadToJoin)
{
    Squad = SquadToJoin;
    Squad->AddMember(this);
}

void AAIGeneric::LeaveSquad()
{
    Squad->RemoveMember(this);
    Squad = NULL;
}
