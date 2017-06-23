// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AI_Squad.h"
#include "Squad.h"
#include "SquadOrder.h"

bool AAI_Squad::HasMember(AAI_Generic * member)
{
    return Members.Contains(member);
}

class ASquad* AAI_Squad::GetSquadPawn()
{
    return SquadPawn;
}

void AAI_Squad::MoveToGrid()
{
    // This doesn't do anything yet...
}

void AAI_Squad::AddMember(AAI_Generic * member)
{
    Members.Emplace(member);
}

void AAI_Squad::RemoveMember(AAI_Generic * member)
{
    Members.Remove(member);
}

void AAI_Squad::SendOrder(USquadOrder * order)
{
    CurrentOrder = order->GetClass();
}
