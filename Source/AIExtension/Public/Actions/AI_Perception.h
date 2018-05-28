// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actions/AIAction.h"
#include "AI_Perception.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class AIEXTENSION_API UAI_Perception : public UAIAction
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "AI|Perception")
	UAIPerceptionComponent* Perception;

	virtual void OnActivation() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "AI|Perception")
	void OnTargetUpdate(AActor* Target, FAIStimulus Stimulus);
};
