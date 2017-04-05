// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtensionPrivatePCH.h"
#include "TaskManagerComponent.h"


// Sets default values for this component's properties
UTaskManagerComponent::UTaskManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTaskManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UTaskManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (auto& Children : ChildrenTasks)
    {
        if(Children.IsValid()) {
            //Cancel task
            Children->Cancel();
        }
    }
    ChildrenTasks.Empty();
}

// Called every frame
void UTaskManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTaskManagerComponent::AddChildren(UTask* NewChildren)
{
    ChildrenTasks.Add(MakeShareable(NewChildren));
}

void UTaskManagerComponent::RemoveChildren(UTask* Children)
{
    TSharedPtr<UTask> TaskPtr = MakeShareable(Children);
    if (ChildrenTasks.Contains(TaskPtr)) {
        ChildrenTasks.Remove(TaskPtr);
    }
}

UTaskManagerComponent* UTaskManagerComponent::GetTaskOwnerComponent_Implementation()
{
    return this;
}

