// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Squad.h"
#include "AIGeneric.h"
#include "UObject/NoExportTypes.h"
#include "SquadOrder.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API USquadOrder : public UObject
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class ASquad* Squad;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class AAIGeneric* Member;

    UFUNCTION(BlueprintCallable)
    void Apply();

    UFUNCTION(BlueprintCallable)
    void Cancel();
};
