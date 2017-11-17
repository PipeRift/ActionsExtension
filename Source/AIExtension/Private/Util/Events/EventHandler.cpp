// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EventHandler.h"
#include "Runtime/Launch/Resources/Version.h"


template< class UserClass >
void FEventHandler::Bind(UserClass* Context, typename FEventDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr InEventMethod)
{
    //Bind Callback
    EventDelegate = FEventDelegate::CreateUObject(Context, InEventMethod);
}

bool FEventHandler::Start(float InLength)
{
    return StartInternal(InLength);
}

bool FEventHandler::StartInternal(float InLength)
{
    if (IsRunning() || InLength < 0)
	{
        return false;
    }

    bActivated = true;
    bPaused = false;

    Timer = new FTimer();
    Length = InLength;

    return true;
}


void FEventHandler::Pause()
{
    if (!IsRunning())
        return;

    bPaused = true;
}

void FEventHandler::Resume()
{
    if (!IsPaused())
	{
        UE_LOG(LogAIExtension, Warning, TEXT("AI Events: Tried to Resume an event, but it was not paused."));
        return;
    }

    bPaused = false;
}

void FEventHandler::Restart(float InLength)
{
    if (!EventDelegate.IsBound())
	{
        UE_LOG(LogAIExtension, Warning, TEXT("AI Events: Tried to Restart an event that is not bounded."));
        return;
    }

    if (InLength < 0)
	{
        InLength = Length;
    }

    //Reset the Event
    Reset();

    //Start the event again
    StartInternal(InLength);
}

void FEventHandler::Reset()
{
    //Clear the Timer
    Timer = nullptr;
    bActivated = false;
    bPaused = false;
}

void FEventHandler::Tick(float DeltaTime)
{
    if (IsRunning() && !IsPaused())
	{
        Timer->Tick(DeltaTime);

        if (Timer->GetCurrentTime() > Length)
		{
            //Check if Event is over
            OnExecute();
        }
    }
}

void FEventHandler::OnExecute()
{
    Reset();

    if (EventDelegate.IsBound())
	{
        EventDelegate.Execute(Id);
    }
}
