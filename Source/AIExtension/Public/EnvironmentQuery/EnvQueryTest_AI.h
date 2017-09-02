// Copyright 2016-2017 Frontwire Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"

#include "AIGeneric.h"
#include "EnvQueryTest_AI.generated.h"

UCLASS(Abstract)
class AIEXTENSION_API UEnvQueryTest_AI : public UEnvQueryTest
{
    GENERATED_UCLASS_BODY()

protected:
    virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
    virtual void RunAITest(AAIGeneric* AI, FEnvQueryInstance& QueryInstance) const;
};
