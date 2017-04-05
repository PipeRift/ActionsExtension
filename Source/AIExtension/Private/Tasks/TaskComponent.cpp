// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TaskComponent.h"


// Sets default values for this component's properties
UTaskComponent::UTaskComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTaskComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UTaskComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (auto& Children : ChildrenTasks)
    {
        if(Children.IsValid()) {
            //Cancel task
            Children->Cancel();
        }
    }
}

// Called every frame
void UTaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTaskComponent::AddChildren(UTask* NewChildren)
{
    ChildrenTasks.Add(MakeShareable(NewChildren));
}

void UTaskComponent::RemoveChildren(UTask* Children)
{
    ChildrenTasks.Remove(MakeShareable(Children));
}

UTaskComponent* UTaskComponent::GetTaskOwnerComponent_Implementation()
{
    return this;
}

