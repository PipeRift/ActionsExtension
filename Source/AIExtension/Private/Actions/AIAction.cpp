// Copyright 2015-2016 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIAction.h"




void UAIAction::OnActivation()
{
    AI = Cast<AAIGeneric>(GetTaskOwnerActor());
    if (!AI)
        Abort();

    Super::OnActivation();
}
