// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"

#include "ActionOwnerInterface.h"

#include "AISquad.h"
#include "AIGeneric.generated.h"


class UBehaviorTreeComponent;
class UBlackboardComponent;

class UAction;
class UActionManagerComponent;


/**
 * 
 */
UCLASS(Blueprintable)
class AIEXTENSION_API AAIGeneric : public AAIController, public IActionOwnerInterface
{
    GENERATED_UCLASS_BODY()

    AAIGeneric();


private:

	UPROPERTY(Transient)
	UBlackboardComponent* BlackboardComp;

	/* Cached BT component */
	UPROPERTY(Transient)
	UBehaviorTreeComponent* BehaviorComp;

public:

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
    class UActionManagerComponent* ActionManagerComponent;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite)
    UAIPerceptionComponent* AIPerceptionComponent;


	UPROPERTY(EditAnywhere, Category = Generic)
	class UBehaviorTree* Behavior;

    UPROPERTY(EditAnywhere, Category = Generic)
    ECombatState State;

private:

    /** Handle for efficient management of Respawn timer */
    FTimerHandle TimerHandle_Respawn;


public:

    // Begin AController interface
	virtual void Possess(class APawn* InPawn) override;
	virtual void UnPossess() override;
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	virtual void BeginInactiveState() override;
	// End AController interface


    // Begin ITaskOwnerInterface interface
    virtual const bool AddChildren(UAction* NewChildren) override;
    virtual const bool RemoveChildren(UAction* Children) override;
    virtual UActionManagerComponent* GetTaskOwnerComponent() override;
    // End ITaskOwnerInterface interface


	void Respawn();

    /** Set the current State of this AI */
    void SetState(const ECombatState InState);

    /** Get the current State of this AI */
    const ECombatState GetState();



    /***************************************
    * Squads                               *
    ***************************************/

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Squad)
    AAISquad* Squad;


    UFUNCTION(BlueprintCallable, Category = Squad)
    void JoinSquad(AAISquad* SquadToJoin);

    UFUNCTION(BlueprintCallable, Category = Squad)
    void LeaveSquad();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Squad)
    FORCEINLINE AAISquad* GetSquad() const
    {
        return Squad;
    }

    UFUNCTION(BlueprintCallable, Category = Squad)
    FORCEINLINE bool IsInSquad() const {
        return IsValid(Squad) && Squad->HasMember(this);
    }

    UFUNCTION(BlueprintCallable, Category = Squad)
    FORCEINLINE UClass* GetOrder() const {
        if (!IsInSquad() || !Squad->CurrentOrder)
        {
            return NULL;
        }

        return Squad->CurrentOrder->GetClass();
    }



    /***************************************
    * INLINES                              *
    ***************************************/

    /** Returns Blackboard component **/
    FORCEINLINE UBlackboardComponent* GetBlackboard() const { return BlackboardComp; }
    /** Returns Behavior component **/
    FORCEINLINE UBehaviorTreeComponent* GetBehavior() const { return BehaviorComp; }
};
