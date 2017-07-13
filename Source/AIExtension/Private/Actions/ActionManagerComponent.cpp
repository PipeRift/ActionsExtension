// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "ActionManagerComponent.h"


// Sets default values for this component's properties
UActionManagerComponent::UActionManagerComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // ...
}


// Called when the game starts
void UActionManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...
    
}


void UActionManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    for (auto* Children : ChildrenTasks)
    {
        if(Children) {
            //Cancel task
            Children->Cancel();
        }
    }
}

// Called every frame
void UActionManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // ...
}

const bool UActionManagerComponent::AddChildren(UAction* NewChildren)
{
    return ChildrenTasks.AddUnique(NewChildren) != INDEX_NONE;
}

const bool UActionManagerComponent::RemoveChildren(UAction* Children)
{
    return ChildrenTasks.Remove(Children) > 0;
}

UActionManagerComponent* UActionManagerComponent::GetTaskOwnerComponent()
{
    return this;
}

