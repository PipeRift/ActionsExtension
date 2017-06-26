// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Squad.h"
#include "AI_Generic.h"
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
    class AAI_Generic* Member;

    UFUNCTION(BlueprintCallable)
    void Apply();

    UFUNCTION(BlueprintCallable)
    void Cancel();
};
