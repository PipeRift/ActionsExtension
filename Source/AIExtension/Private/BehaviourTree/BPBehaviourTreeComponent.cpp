// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"

#include "BPBT_Node.h"

#include "BPBehaviourTreeComponent.h"

UBPBehaviourTreeComponent::UBPBehaviourTreeComponent(const FObjectInitializer& ObjectInitializer) 
    : Super(ObjectInitializer)
{
}

UBPBT_Node* UBPBehaviourTreeComponent::Node(UObject* WorldContextObject, TSubclassOf<class UBPBT_Node> ItemType, APlayerController* OwningPlayer)
{
    return nullptr;
}
