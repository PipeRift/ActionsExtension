// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"

#include "AISquad.h"
#include "AIGeneric.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;

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
    GENERATED_UCLASS_BODY()

    AAIGeneric();


private:

	UPROPERTY(transient)
	UBlackboardComponent* BlackboardComp;

	/* Cached BT component */
	UPROPERTY(transient)
	UBehaviorTreeComponent* BehaviorComp;

public:

	UPROPERTY(EditAnywhere, Category=Behavior)
	class UBehaviorTree* Behavior;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UAIPerceptionComponent* AIPerceptionComponent;


    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
    ECombatState State;

    // Begin AController interface
	virtual void Possess(class APawn* InPawn) override;
	virtual void UnPossess() override;
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	virtual void BeginInactiveState() override;
	// End APlayerController interface

	void Respawn();

	/** Handle for efficient management of Respawn timer */
	FTimerHandle TimerHandle_Respawn;
    
public:

	/** Returns Blackboard component **/
	FORCEINLINE UBlackboardComponent* GetBlackboard() const { return BlackboardComp; }
	/** Returns Behavior component **/
	FORCEINLINE UBehaviorTreeComponent* GetBehavior() const { return BehaviorComp; }



    /***************************************
    * Squads                               *
    ***************************************/

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AAISquad* Squad;


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
