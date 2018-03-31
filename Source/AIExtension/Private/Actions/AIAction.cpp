// Copyright 2015-2016 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIAction.h"




void UAIAction::OnActivation()
{
    AI = Cast<AAIGeneric>(GetActionOwnerActor());
    if (!AI)
        Abort();

    Super::OnActivation();
}
