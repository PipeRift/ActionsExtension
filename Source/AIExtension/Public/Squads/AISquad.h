// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AISquad.generated.h"

class ASquad;
class USquadOrder;
class AAIGeneric;

/**
 * 
 */
UCLASS()
class AIEXTENSION_API AAISquad : public AAIController
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<AAIGeneric*> Members;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector PositionIncrement;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<USquadOrder> CurrentOrder;

    UFUNCTION(BlueprintCallable)
    FORCEINLINE bool HasMember(const AAIGeneric* member) const {
        return Members.Contains(member);
    }

    UFUNCTION(BlueprintCallable)
    ASquad* GetSquadPawn();

    UFUNCTION(BlueprintCallable)
    void AddMember(AAIGeneric* member);

    UFUNCTION(BlueprintCallable)
    void RemoveMember(AAIGeneric* member);

    UFUNCTION(BlueprintCallable)
    void SendOrder(USquadOrder* order);
};
