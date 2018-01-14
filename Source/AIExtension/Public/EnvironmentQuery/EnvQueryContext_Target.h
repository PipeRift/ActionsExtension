// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Target.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UEnvQueryContext_Target : public UEnvQueryContext
{
    GENERATED_UCLASS_BODY()

    virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
