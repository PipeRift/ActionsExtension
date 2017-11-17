// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EventComponent.h"


// Sets default values for this component's properties
UEventComponent::UEventComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    DefaultLength = 1;

    EventHandler = FEventHandler(this, 0);
    EventHandler.Bind<UEventComponent>(this, &UEventComponent::OnExecute);
}

void UEventComponent::BeginPlay()
{
    if (bAutoActivate) {
        Start();
    }
}

void UEventComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    EventHandler.Tick(DeltaTime);
}

void UEventComponent::EndPlay(const EEndPlayReason::Type reason)
{
    EventHandler.Reset();
}

bool UEventComponent::Start(float Length)
{
    Activate();

    if (Length < 0) {
        Length = DefaultLength;
    }
    return EventHandler.Start(Length);
}

void UEventComponent::Pause()
{
    EventHandler.Pause();
}

void UEventComponent::Resume()
{
    EventHandler.Resume();
}

void UEventComponent::Restart(float Length)
{
    EventHandler.Restart(Length);
}

void UEventComponent::Reset()
{
    EventHandler.Reset();
}

void UEventComponent::OnExecute(int Id)
{
    if (bLooping) {
        Start();
    }
    Execute.Broadcast();
}

bool UEventComponent::IsRunning() {
    return EventHandler.IsRunning();
}

bool UEventComponent::IsPaused()
{
    return EventHandler.IsPaused();
}

float UEventComponent::GetLength()
{
    return EventHandler.GetLength();
}
