// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Runtime/AIModule/Classes/Perception/AIPerceptionComponent.h"
#include "EAIEnums.h"
#include "AI_Generic.generated.h"

/**
 * 
 */
class AAI_Squad;

UCLASS()
class AIEXTENSION_API AAI_Generic : public AAIController
{
    GENERATED_BODY()

public:
    AAI_Generic(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class AAI_Squad* Squad;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class UAIPerceptionComponent* AIPerceptionComponent;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
    ECombatState State = ECombatState::EPassive;

    UFUNCTION(BlueprintCallable)
    void JoinSquad(class AAI_Squad* SquadToJoin);

    UFUNCTION(BlueprintCallable)
    void LeaveSquad();

    UFUNCTION(BlueprintCallable)
    bool IsInSquad();

    UFUNCTION(BlueprintCallable)
    TSubclassOf<USquadOrder> GetOrder();
};
