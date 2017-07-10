// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "Squad.h"
#include "SquadOrder.h"
#include "AIGeneric.h"

#include "AISquad.h"


void AAISquad::AddMember(AAIGeneric* Member)
{
    Members.Emplace(Member);
}

void AAISquad::RemoveMember(AAIGeneric* Member)
{
    Members.Remove(Member);
}

void AAISquad::SetLeader(AAIGeneric* NewLeader)
{
    if(Members.Contains(NewLeader))
    {
        Leader = NewLeader;
    }
}

void AAISquad::SendOrder(USquadOrder * order)
{
    CurrentOrder = order->GetClass();
}
