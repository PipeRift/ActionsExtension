// Copyright 2015-2016 Piperift. All Rights Reserved.

#pragma once

#include "Action.h"
#include "AIGeneric.h"

#include "AIAction.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API UAIAction : public UAction
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	AAIGeneric* AI;

	virtual void OnActivation() override;
};
