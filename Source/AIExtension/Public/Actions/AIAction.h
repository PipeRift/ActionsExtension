// Copyright 2015-2016 Piperift. All Rights Reserved.

#pragma once

#include "Actions/Action.h"
#include "AIGeneric.h"

#include "AIAction.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API UAIAction : public UAction
{
    GENERATED_BODY()

    UPROPERTY()
    AAIGeneric* AI;

	virtual void OnActivation() override {
        AI = Cast<AAIGeneric>(GetTaskOwnerActor());
        if(!AI)
            Abort();

        Super::OnActivation();
    }
	
	
};
