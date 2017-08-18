// Copyright 2016-2017 Frontwire Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AITargetEvaluatorTest.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew)
class AIEXTENSION_API UAITargetEvaluatorTest : public UObject
{
	GENERATED_BODY()
	
public:

    float Calculate(APawn* Target) {
        return FMath::Clamp(CalculateRawScore(Target), 0.0f, 1.0f);
    }

protected:

	virtual float CalculateRawScore(APawn* Target) {
        return ExecCalculateScore();
    }
	
    UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "CalculateScore"))
    float ExecCalculateScore();
	
};
