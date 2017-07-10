// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AISquad.generated.h"

class USquadOrder;
class AAIGeneric;


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
UCLASS()
class AIEXTENSION_API AAISquad : public AInfo
{
    GENERATED_BODY()

protected:

    UPROPERTY(EditAnywhere, Category = Generic)
    ECombatState State;

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<AAIGeneric*> Members;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AAIGeneric* Leader;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector PositionIncrement;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<USquadOrder> CurrentOrder;


    UFUNCTION(BlueprintCallable)
    FORCEINLINE bool HasMember(const AAIGeneric* member) const {
        return Members.Contains(member);
    }

    UFUNCTION(BlueprintCallable)
    void AddMember(AAIGeneric* Member);

    UFUNCTION(BlueprintCallable)
    void RemoveMember(AAIGeneric* Member);

    UFUNCTION(BlueprintCallable)
    virtual void SetLeader(AAIGeneric* NewLeader);

    UFUNCTION(BlueprintCallable)
    inline AAIGeneric* GetLeader() const
    {
        return Leader;
    }

    UFUNCTION(BlueprintCallable)
    void SendOrder(USquadOrder* Order);



    /***************************************
    * INLINES                              *
    ***************************************/

    /** Get the current State of this AI */
    FORCEINLINE const ECombatState GetState() {
        return State;
    }
};
