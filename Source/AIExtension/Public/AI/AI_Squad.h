// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI_Generic.h"
#include "AI_Squad.generated.h"

class ASquad;
class USquadOrder;
/**
 * 
 */
UCLASS()
class AIEXTENSION_API AAI_Squad : public AAIController
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class ASquad* SquadPawn;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<AAI_Generic*> Members;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector PositionIncrement;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<USquadOrder> CurrentOrder;

    UFUNCTION(BlueprintCallable)
    bool HasMember(AAI_Generic* member);

    UFUNCTION(BlueprintCallable)
    class ASquad* GetSquadPawn();

    UFUNCTION(BlueprintCallable)
    void MoveToGrid();

    UFUNCTION(BlueprintCallable)
    void AddMember(AAI_Generic* member);

    UFUNCTION(BlueprintCallable)
    void RemoveMember(AAI_Generic* member);

    UFUNCTION(BlueprintCallable)
    void SendOrder(USquadOrder* order);
};
