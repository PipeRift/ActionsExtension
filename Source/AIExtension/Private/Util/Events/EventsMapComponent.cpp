// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EventsMapComponent.h"


// Sets default values for this component's properties
UEventsMapComponent::UEventsMapComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    DefaultLength = 1;
}

void UEventsMapComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    for (auto& Pair : Events)
    {
        if (Pair.Value.IsValid()) {
            Pair.Value.Tick(DeltaTime);
        }
    }
}

void UEventsMapComponent::Start(int Id, float Length)
{
    if (Length < 0) {
        Length = DefaultLength;
    }

    if (!Events.Contains(Id) || !Events[Id].IsValid()){
        Events.Add(Id, FEventHandler(this, Id));

        // Create and setup event
        FEventHandler& Event = Events[Id];
        Event.Bind<UEventsMapComponent>(this, &UEventsMapComponent::OnExecute);

        Event.Start(Length);
    }
}

void UEventsMapComponent::Pause(int Id)
{
    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return;

    Event.Pause();
}

void UEventsMapComponent::Resume(int Id)
{
    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return;

    Event.Resume();
}

void UEventsMapComponent::Restart(int Id, float Length, bool bStartIfNeeded)
{
    if (!Events.Contains(Id)) {
        if (bStartIfNeeded) {
            Start(Id, Length);
        }
        return;
    }

    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return;

    Event.Restart(Length);
}

void UEventsMapComponent::Reset(int Id)
{
    if (!Events.Contains(Id))
        return;

    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return;

    Event.Reset();
}

void UEventsMapComponent::OnExecute(int Id)
{
    if (Events.Contains(Id))
    {
        Execute.Broadcast(Id);
    }
}

bool UEventsMapComponent::IsRunning(int Id) {
    if (!Events.Contains(Id))
        return false;

    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return false;
    return Event.IsRunning();
}

bool UEventsMapComponent::IsPaused(int Id)
{
    if (!Events.Contains(Id))
        return false;

    FEventHandler& Event = Events[Id];
    if (!Event.IsValid()) 
        return false;
    return Event.IsPaused();
}

float UEventsMapComponent::GetLength(int Id)
{
    if (!Events.Contains(Id))
        return false;

    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return -1;
    return Event.GetLength();
}

bool UEventsMapComponent::GetEventHandler(FEventHandler& OutEvent, int Id)
{
    if (!Events.Contains(Id))
        return false;

    FEventHandler& Event = Events[Id];
    if (!Event.IsValid())
        return false;

    OutEvent = Event;
    return true;
}
