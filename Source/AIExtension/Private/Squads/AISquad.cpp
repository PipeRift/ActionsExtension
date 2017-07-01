// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "Squad.h"
#include "SquadOrder.h"
#include "AIGeneric.h"

#include "AISquad.h"


class ASquad* AAISquad::GetSquadPawn()
{
    return Cast<ASquad>(GetPawn());
}

void AAISquad::AddMember(AAIGeneric * member)
{
    Members.Emplace(member);
}

void AAISquad::RemoveMember(AAIGeneric * member)
{
    Members.Remove(member);
}

void AAISquad::SendOrder(USquadOrder * order)
{
    CurrentOrder = order->GetClass();
}
