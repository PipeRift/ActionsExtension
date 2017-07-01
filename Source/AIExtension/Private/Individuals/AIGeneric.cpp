// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "AIGeneric.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"


AAIGeneric::AAIGeneric(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    AIPerceptionComponent = ObjectInitializer.CreateDefaultSubobject<UAIPerceptionComponent>(this, TEXT("Perception"));
    
 	BlackboardComp = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoard"));
 	
	BrainComponent = BehaviorComp = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("Behavior"));	


    State = ECombatState::Passive;
}

void AAIGeneric::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	// start behavior
	if (Behavior)
	{
		if (Behavior->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*Behavior->BlackboardAsset);
		}

		BehaviorComp->StartTree(*Behavior);
	}
}

void AAIGeneric::UnPossess()
{
	Super::UnPossess();

	BehaviorComp->StopTree();
}

void AAIGeneric::BeginInactiveState()
{
	Super::BeginInactiveState();

	AGameStateBase const* const GameState = GetWorld()->GetGameState();

	const float MinRespawnDelay = GameState ? GameState->GetPlayerRespawnDelay(this) : 1.0f;

	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &AAIGeneric::Respawn, MinRespawnDelay);
}

void AAIGeneric::Respawn()
{
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

void AAIGeneric::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	// Stop the behavior tree/logic
	BehaviorComp->StopTree();

	// Stop any movement we already have
	StopMovement();

	// Cancel the respawn timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Respawn);

	// Clear any target
	//SetEnemy(NULL);	
}


/***************************************
* Squads                               *
***************************************/

void AAIGeneric::JoinSquad(AAISquad * SquadToJoin)
{
    Squad = SquadToJoin;
    Squad->AddMember(this);
}

void AAIGeneric::LeaveSquad()
{
    Squad->RemoveMember(this);
    Squad = NULL;
}
