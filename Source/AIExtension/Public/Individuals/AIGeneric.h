// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

#include "ActionOwnerInterface.h"
#include "FactionAgentInterface.h"

#include "AIGeneric.generated.h"

class UAISquad;
class UBehaviorTreeComponent;
class UBehaviorTree;

class UAction;
class UActionManagerComponent;


/**
 *
**/
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
class AIEXTENSION_API AAIGeneric : public AAIController, public IActionOwnerInterface, public IFactionAgentInterface
{
    GENERATED_UCLASS_BODY()

    AAIGeneric();
    virtual void OnConstruction(const FTransform& Transform) override;

private:

    UPROPERTY(Transient)
    UBlackboardComponent* BlackboardComp;

    /* Cached BT component */
    UPROPERTY(Transient)
    UBehaviorTreeComponent* BehaviorComp;

public:

    UPROPERTY(Category = AI, VisibleAnywhere, BlueprintReadOnly)
    UActionManagerComponent* ActionManagerComponent;

    /** Targets */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Targets")
    bool bScanPotentialTargets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Targets")
    UEnvQuery* ScanQuery;

    /* Potential target scan rate in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Targets")
    float TargetScanRate;


    /** Behaviors */
    UPROPERTY(EditAnywhere, Category = "AI|Behaviors", meta = (DisplayName = "Base"))
    UBehaviorTree* BaseBehavior;
    UPROPERTY(EditAnywhere, Category = "AI|Behaviors", meta = (DisplayName = "Combat", DisplayThumbnail = false))
    UBehaviorTree* CombatBehavior;
    UPROPERTY(EditAnywhere, Category = "AI|Behaviors", meta = (DisplayName = "Alert", DisplayThumbnail = false))
    UBehaviorTree* AlertBehavior;
    UPROPERTY(EditAnywhere, Category = "AI|Behaviors", meta = (DisplayName = "Suspicion", DisplayThumbnail = false))
    UBehaviorTree* SuspicionBehavior;
    UPROPERTY(EditAnywhere, Category = "AI|Behaviors", meta = (DisplayName = "Passive", DisplayThumbnail = false))
    UBehaviorTree* PassiveBehavior;

    UPROPERTY(EditAnywhere, Category = AI)
    ECombatState State;

private:

    UPROPERTY(EditAnywhere, Category = AI)
    float ReactionTime;

    UPROPERTY(Transient)
    TSet<APawn*> PotentialTargets;

    /** Handle for efficient management of Reaction timer */
    FTimerHandle TimerHandle_Reaction;

    /** Handle for efficient management of Respawn timer */
    FTimerHandle TimerHandle_Respawn;

    /** Handle for efficient management of Target Scan timer */
    FTimerHandle TimerHandle_TargetScan;


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
    virtual UActionManagerComponent* GetActionOwnerComponent() override;
    // End ITaskOwnerInterface interface


    UFUNCTION(BlueprintCallable, Category = AI)
    void Respawn();

    /* Start combat with InTarget */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat System")
    void StartCombat(APawn* InTarget);

    UFUNCTION(BlueprintCallable, Category = "AI|Combat System")
    void FinishCombat(ECombatState DestinationState = ECombatState::Alert);

    /** Set the current State of this AI */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat System")
    const bool SetState(const ECombatState InState);

    /** Get the current State of this AI */
    UFUNCTION(BlueprintPure, Category = "AI|Combat System")
    const ECombatState GetState() const;

    UFUNCTION(BlueprintPure, Category = "AI|Combat System")
    FORCEINLINE bool IsInCombat() const { return GetTarget() && GetState() == ECombatState::Combat; }

    /** Get the current combat Target of this AI */
    UFUNCTION(BlueprintPure, Category = "AI|Combat System")
    FORCEINLINE APawn* GetTarget() const {
        return Cast<APawn>(BlackboardComp->GetValueAsObject(TEXT("Target")));
    }

    /** Get the AI of the combat Target if existing */
    UFUNCTION(BlueprintPure, Category = "AI|Combat System")
    FORCEINLINE AAIGeneric* GetAITarget() const {
        const APawn* Target = GetTarget();
        return Target ? Cast<AAIGeneric>(Target->GetController()) : nullptr;
    }

protected:

    /** Check if this AI can start combat with Target */
    virtual const bool CanAttack(APawn* Target) const;

    /** Check if this AI can start combat with Target */
    UFUNCTION(BlueprintNativeEvent, Category = "Combat", meta = (DisplayName = "Can Attack"))
    bool ExecCanAttack(APawn* Target) const;

    /** Called after combat started */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI|Combat System")
    void OnCombatStarted(APawn* Target);

    /** Check after combat finished */
    UFUNCTION(BlueprintImplementableEvent, Category = "AI|Combat System")
    void OnCombatFinished(APawn* Target);


public:

    /***************************************
    * Potential Targets                    *
    ***************************************/

    /** Get all potential targets */
    const TSet<APawn*>& GetPotentialTargetsRef() const {
        return PotentialTargets;
    }

    /** Get all potential targets */
    UFUNCTION(BlueprintPure, Category = "AI|Combat System")
    FORCEINLINE TSet<APawn*> GetPotentialTargets() const {
        return PotentialTargets;
    }

    /** Add a new potential target */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat System")
    void AddPotentialTarget(APawn* Target);

    /** Remove a new potential target */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat System")
    void RemovePotentialTarget(APawn* Target);

    /** Search for a new target */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat System")
    void TryScanPotentialTarget();

    void OnTargetSelectionFinished(TSharedPtr<FEnvQueryResult> Result);

    const bool IsValidScanQuery() const;

    /***************************************
    * Squads                               *
    ***************************************/

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = AI)
    AAISquad* Squad;


    UFUNCTION(BlueprintCallable, Category = Squad)
    void JoinSquad(AAISquad* SquadToJoin);

    UFUNCTION(BlueprintCallable, Category = Squad)
    void LeaveSquad();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Squad)
    FORCEINLINE AAISquad* GetSquad() const {
        return Squad;
    }

    UFUNCTION(BlueprintCallable, Category = Squad)
    bool IsInSquad() const;

    UFUNCTION(BlueprintPure, Category = Squad)
    UClass* GetSquadOrder() const;


    /***************************************
    * Factions                             *
    ***************************************/

    /** Retrieve faction identifier in form of Faction */
    UFUNCTION(BlueprintPure, Category = Faction)
    virtual FFaction GetFaction() const override;

    /** Assigns faction */
    UFUNCTION(BlueprintCallable, Category = Faction)
    virtual void SetFaction(const FFaction& InFaction) override;

    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override { SetFaction(FFaction(NewTeamID)); }
    virtual FGenericTeamId GetGenericTeamId() const override { return GetFaction().GetTeam(); }


    /***************************************
    * INLINES                              *
    ***************************************/

    /** Returns Blackboard component **/
    UFUNCTION(BlueprintPure, Category = AI)
    FORCEINLINE UBlackboardComponent* GetBlackboard() const { return BlackboardComp; }

    /** Returns Behavior component **/
    UFUNCTION(BlueprintPure, Category = AI)
    FORCEINLINE UBehaviorTreeComponent* GetBehavior() const { return BehaviorComp; }

private:

    /***************************************
    * HELPERS                              *
    ***************************************/

    void SetDynamicSubBehavior(FName GameplayTag, UBehaviorTree* SubBehavior);

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR
};
