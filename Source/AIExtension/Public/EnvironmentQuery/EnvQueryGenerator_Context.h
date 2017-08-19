// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvQueryGenerator_Context.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "From Context"))
class AIEXTENSION_API UEnvQueryGenerator_Context : public UEnvQueryGenerator
{
	GENERATED_UCLASS_BODY()

    /** Context from where we acquire actors and locations */
    UPROPERTY(EditAnywhere, Category = Generator)
    TSubclassOf<UEnvQueryContext> Source;

    virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

    virtual FText GetDescriptionTitle() const override;
    virtual FText GetDescriptionDetails() const override;
	
	
	
};
