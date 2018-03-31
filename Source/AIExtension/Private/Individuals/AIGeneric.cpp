// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "AIGeneric.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Actions/ActionManagerComponent.h"
#include "EnvQueryGenerator_Context.h"
#include "EnvQueryContext_PotentialTargets.h"
#include "AISquad.h"

#define LOCTEXT_NAMESPACE "AAIGeneric"


static const TCHAR* TargetFilterAsset = TEXT("/AIExtension/Base/Individuals/EQS/TargetFilter");

AAIGeneric::AAIGeneric(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    ActionManagerComponent  = ObjectInitializer.CreateDefaultSubobject<UActionManagerComponent>(this, TEXT("Action Manager"));
    
 	BlackboardComp = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoard"));
	BrainComponent = BehaviorComp = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("Behavior"));	

    //static ConstructorHelpers::FObjectFinder<UBehaviorTree> BaseBehaviorAsset(TEXT("/AIExtension/Base/Individuals/BT_Generic"));
    //BaseBehavior = BaseBehaviorAsset.Object;

    State = ECombatState::Passive;
    ReactionTime = 0.2f;

    bScanPotentialTargets = true;
    TargetScanRate = 0.3f;

    static ConstructorHelpers::FObjectFinder<UEnvQuery> ScanQueryAsset(TargetFilterAsset);
    ScanQuery = ScanQueryAsset.Object;
}

void AAIGeneric::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
}

void AAIGeneric::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	// start behavior
	if (BaseBehavior)
	{
		if (BaseBehavior->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*BaseBehavior->BlackboardAsset);
            BlackboardComp->SetValueAsObject(TEXT("SelfActor"), InPawn);
		}

		BehaviorComp->StartTree(*BaseBehavior);
        SetDynamicSubBehavior(FAIExtensionModule::FBehaviorTags::Combat,    CombatBehavior);
        SetDynamicSubBehavior(FAIExtensionModule::FBehaviorTags::Alert,     AlertBehavior);
        SetDynamicSubBehavior(FAIExtensionModule::FBehaviorTags::Suspicion, SuspicionBehavior);
        SetDynamicSubBehavior(FAIExtensionModule::FBehaviorTags::Passive,   PassiveBehavior);
	}

    GetWorldTimerManager().SetTimer(TimerHandle_TargetScan, this, &AAIGeneric::TryScanPotentialTarget, TargetScanRate);
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


const bool AAIGeneric::AddChildren(UAction* NewChildren)
{
    check(ActionManagerComponent);
    return ActionManagerComponent->AddChildren(NewChildren);
}

const bool AAIGeneric::RemoveChildren(UAction* RemChildren)
{
    check(ActionManagerComponent);
    return ActionManagerComponent->RemoveChildren(RemChildren);
}

UActionManagerComponent* AAIGeneric::GetActionOwnerComponent()
{
    check(ActionManagerComponent);
    return ActionManagerComponent;
}


void AAIGeneric::Respawn()
{
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

void AAIGeneric::StartCombat(APawn* InTarget)
{
    if (!CanAttack(InTarget) || (IsInCombat() && InTarget == GetTarget()))
        return;

    //Is already in combat? Finish it!
    if (IsInCombat())
        FinishCombat(ECombatState::Alert);

    if (SetState(ECombatState::Combat))
    {
        BlackboardComp->SetValueAsObject(TEXT("Target"), InTarget);

        AddPotentialTarget(InTarget);
        OnCombatStarted(InTarget);
    }
}

void AAIGeneric::FinishCombat(ECombatState DestinationState /*= ECombatState::Alert*/)
{
    if (!IsInCombat() || DestinationState == ECombatState::Combat)
        return;

    APawn* CurrentTarget = GetTarget();
    SetState(DestinationState);

    OnCombatStarted(CurrentTarget);
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
    FinishCombat();
}

const bool AAIGeneric::CanAttack(APawn* Target) const
{
    if (!Target || Target == GetPawn() || !IsHostileTowards(*Target))
        return false;
    return ExecCanAttack(Target);
}

bool AAIGeneric::ExecCanAttack_Implementation(APawn* Target) const
{
    return IsHostileTowards(*Target);
}


/***************************************
* Potential Targets                    *
***************************************/

void AAIGeneric::AddPotentialTarget(APawn* Target)
{
    if (CanAttack(Target))
    {
        PotentialTargets.Add(Target);

        //Start combat if this is the first potential target
        if (!IsInCombat() && GetTarget() != Target)
        {
            //Wait for reaction time
            FTimerDelegate Callback;
            Callback.BindLambda([Target, this]
            {
                //Repeat check later
                if (!IsInCombat() && GetTarget() != Target)
                {
                    StartCombat(Target);
                }
            });
            GetWorldTimerManager().SetTimer(TimerHandle_Reaction, Callback, ReactionTime, false);
        }
    }
}

void AAIGeneric::RemovePotentialTarget(APawn* Target)
{
    if (!Target)
        return;

    PotentialTargets.Remove(Target);

    //Finish combat if the target is the same
    if (IsInCombat() && GetTarget() == Target)
    {
        FinishCombat();
        if (PotentialTargets.Num() > 0)
        {
            StartCombat(PotentialTargets.Array()[0]);
        }
    }
}

void AAIGeneric::TryScanPotentialTarget()
{
    if(bScanPotentialTargets && IsValidScanQuery())
    {
        FEnvQueryRequest TargetFilterRequest = FEnvQueryRequest(ScanQuery, this);
        TargetFilterRequest.Execute(EEnvQueryRunMode::SingleResult, this, &AAIGeneric::OnTargetSelectionFinished);
    }

    GetWorldTimerManager().SetTimer(TimerHandle_TargetScan, this, &AAIGeneric::TryScanPotentialTarget, TargetScanRate);
}


void AAIGeneric::OnTargetSelectionFinished(TSharedPtr<FEnvQueryResult> Result)
{
    TArray<AActor*> Results;
    Result->GetAllAsActors(Results);

    //No results!
    if (Results.Num() < 1)
        return;

    APawn* NewTarget = Cast<APawn>(Results[0]);

    //No results!
    if (!NewTarget)
        return;

    //A more important target has been found, attack him
    StartCombat(NewTarget);
}

const bool AAIGeneric::IsValidScanQuery() const
{
    if (!ScanQuery)
        return false;

    const TArray<UEnvQueryOption*>& Options = ScanQuery->GetOptions();

    //No Generators!
    if (Options.Num() < 1)
        return false;

    for (auto It = Options.CreateConstIterator(); It; ++It)
    {
        const UEnvQueryGenerator_Context* ContextGenerator = Cast<UEnvQueryGenerator_Context>((*It)->Generator);
        if (!ContextGenerator || ContextGenerator->Source != UEnvQueryContext_PotentialTargets::StaticClass())
        {
            //Any generator is not Context type or its context is not potential targets
            return false;
        }
    }
    return true;
}


/***************************************
* Squads                               *
***************************************/

void AAIGeneric::JoinSquad(AAISquad * SquadToJoin)
{
    if (SquadToJoin != NULL)
    {
        Squad = SquadToJoin;
        Squad->AddMember(this);
    }
}

void AAIGeneric::LeaveSquad()
{
    Squad->RemoveMember(this);
    Squad = NULL;
}

bool AAIGeneric::IsInSquad() const
{
	return IsValid(Squad) && Squad->HasMember(this);
}

UClass* AAIGeneric::GetSquadOrder() const
{
    if (!IsInSquad() || !Squad->CurrentOrder)
    {
        return NULL;
    }

    return Squad->CurrentOrder->GetClass();
}


/***************************************
* Squads                               *
***************************************/

FFaction AAIGeneric::GetFaction() const
{
    FFaction EventFaction;
    Execute_EventGetFaction(this, EventFaction);

	if (EventFaction.IsNone())
	{
		return IFactionAgentInterface::Execute_GetFaction(GetPawn());
	}
	else
	{
		return EventFaction;
	}
}

void AAIGeneric::SetFaction(const FFaction & InFaction)
{
    Execute_EventSetFaction(this, InFaction);
	IFactionAgentInterface::Execute_SetFaction(GetPawn(), InFaction);
}


void AAIGeneric::SetDynamicSubBehavior(FName GameplayTag, UBehaviorTree* SubBehavior)
{
    if (BehaviorComp && SubBehavior)
    {
        BehaviorComp->SetDynamicSubtree(FGameplayTag::RequestGameplayTag(GameplayTag), SubBehavior);
    }
}

const bool AAIGeneric::SetState(ECombatState InState)
{
    //Only allow State change when squad is not in a more important state
    if (!IsInSquad() || Squad->GetState() < InState) {
        State = InState;
        BlackboardComp->SetValueAsEnum(TEXT("CombatState"), (uint8)InState);
        return true;
    }
    return false;
}

const ECombatState AAIGeneric::GetState() const
{
    if (IsInSquad())
    {
        return FMath::Max(Squad->GetState(), State);
    }
    return  State;
}

#if WITH_EDITOR
void AAIGeneric::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    if (PropertyChangedEvent.Property != nullptr)
    {
        const FName PropertyName(PropertyChangedEvent.Property->GetFName());

        if (PropertyName == GET_MEMBER_NAME_CHECKED(AAIGeneric, ScanQuery))
        {
            if (!IsValidScanQuery())
            {
                if (!ScanQuery)
                    return;

                FText Message = FText::Format(LOCTEXT("TargetFilterEQSNotValid", "EnvQuery {0} must use 'EnvQueryGenerator_Context' with 'EnvQueryContext_PotentialTargets' as Source."), FText::FromString(ScanQuery->GetName()));
                UE_LOG(LogTemp, Warning, TEXT("%s"), *Message.ToString());

                FNotificationInfo Info(Message);
                Info.ExpireDuration = 3.0f;
                FSlateNotificationManager::Get().AddNotification(Info);

                //Set default
                static ConstructorHelpers::FObjectFinder<UEnvQuery> ScanQueryAsset(TargetFilterAsset);
                ScanQuery = ScanQueryAsset.Object;
            }
        }
    }
}
#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE
