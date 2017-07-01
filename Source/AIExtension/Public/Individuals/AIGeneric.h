// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Perception/AIPerceptionComponent.h"

#include "AISquad.h"
#include "AIGeneric.generated.h"


/**
 *
 */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
    Passive,
    Suspicion,
    Alert,
    Combat
};


/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API AAIGeneric : public AAIController
{
    GENERATED_BODY()

public:
    AAIGeneric(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AAISquad* Squad;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UAIPerceptionComponent* AIPerceptionComponent;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
    ECombatState State;

    UFUNCTION(BlueprintCallable)
    void JoinSquad(class AAISquad* SquadToJoin);

    UFUNCTION(BlueprintCallable)
    void LeaveSquad();

    UFUNCTION(BlueprintCallable)
    FORCEINLINE bool IsInSquad() const {
        return IsValid(Squad) && Squad->HasMember(this);
    }

    UFUNCTION(BlueprintCallable)
    FORCEINLINE UClass* GetOrder() const {
        if(!IsInSquad() || !Squad->CurrentOrder)
            return NULL;

        return Squad->CurrentOrder->GetClass();
    }
};
