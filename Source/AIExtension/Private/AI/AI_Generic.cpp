// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "AI_Generic.h"
#include "AI_Squad.h"

AAI_Generic::AAI_Generic(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
}

void AAI_Generic::JoinSquad(AAI_Squad * SquadToJoin)
{
    Squad = SquadToJoin;
    Squad->AddMember(this);
}

void AAI_Generic::LeaveSquad()
{
    Squad->RemoveMember(this);
    Squad = NULL;
}

bool AAI_Generic::IsInSquad()
{
    if (!IsValid(Squad))
    {
        return false;
    }

    return Squad->HasMember(this);
}

TSubclassOf<USquadOrder> AAI_Generic::GetOrder()
{
    if (!IsInSquad())
    {
        return NULL;
    }

    return Squad->CurrentOrder;
}
