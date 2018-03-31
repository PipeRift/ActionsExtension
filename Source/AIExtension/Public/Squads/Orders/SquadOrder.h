// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIGeneric.h"
#include "UObject/NoExportTypes.h"
#include "SquadOrder.generated.h"

class AAISquad;


/**
 * 
 */
UCLASS()
class AIEXTENSION_API USquadOrder : public UObject
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AAISquad* Squad;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AAIGeneric* Member;

    UFUNCTION(BlueprintCallable)
    void Apply();

    UFUNCTION(BlueprintCallable)
    void Cancel();
};
