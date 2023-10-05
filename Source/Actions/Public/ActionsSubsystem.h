// Copyright 2015-2023 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <Subsystems/GameInstanceSubsystem.h>
#include <Tickable.h>

#include "ActionsSubsystem.generated.h"


class UAction;

/**
 * Contains a list of actions with the same TickRate
 */
USTRUCT()
struct FActionsTickGroup
{
	GENERATED_BODY()

	UPROPERTY()
	float TickRate = 0.f;

	UPROPERTY(Transient)
	float TickTimeElapsed = 0.f;

	UPROPERTY()
	TArray<UAction*> Actions;


	FActionsTickGroup(float TickRate = 0.f) : TickRate(TickRate) {}

	inline void Tick(float DeltaTime);

private:
	inline void DelayedTick(float DeltaTime);

public:
	bool operator==(const FActionsTickGroup& Other) const
	{
		return FMath::IsNearlyEqual(TickRate, Other.TickRate);
	}
	bool operator!=(const FActionsTickGroup& Other) const
	{
		return !(*this == Other);
	}

	friend const uint32 GetTypeHash(const FActionsTickGroup& InGroup)
	{
		return GetTypeHash(InGroup.TickRate);
	}
};

/**
 * Represents a dependency of an objects with all its actions
 * Used to cancel actions whose owner is destroyed
 */
USTRUCT()
struct FActionOwner
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TArray<TObjectPtr<UAction>> Actions;


	FActionOwner(UObject* Owner = nullptr) : Owner(Owner) {}

	void CancelAll(bool bShouldShrink = true);
	void CancelByPredicate(const TFunctionRef<bool(const UAction*)>& Predicate, bool bShouldShrink = true);

	/**
	 * Operator overloading & Hashes
	 */
	bool operator==(const FActionOwner& Other) const
	{
		return Owner == Other.Owner;
	}
	bool operator!=(const FActionOwner& Other) const
	{
		return !(*this == Other);
	}
	friend uint32 GetTypeHash(const FActionOwner& InAction)
	{
		return GetTypeHash(InAction.Owner);
	}
};


/**
 * Actions Subsystem
 * Keeps track of all running actions and their lifetime.
 * It also does a global tick based on tick rate for all actions.
 */
UCLASS()
class ACTIONS_API UActionsSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

	friend UAction;

private:
	UPROPERTY(SaveGame)
	TSet<FActionOwner> ActionOwners;

	UPROPERTY(Transient)
	TArray<FActionsTickGroup> TickGroups;


protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	//~ Begin Tickable GameObject Interface
	virtual void Tick(float DeltaTime) override;

	virtual bool IsTickable() const override
	{
		return ActionOwners.Num() > 0 || TickGroups.Num() > 0;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UActionsSubsystem, STATGROUP_Tickables);
	}
	//~ End Tickable GameObject Interface


	/** Cancel all current actions of the game. Use with care! */
	void CancelAll();

	/** Cancel all actions executing inside an object
	 * @param Owner of the actions to cancel
	 */
	UFUNCTION(BlueprintCallable, Category = ActionSubsystem)
	void CancelAllByOwner(UObject* Object);

	/** Cancel all actions matching a predicate */
	void CancelByPredicate(TFunctionRef<bool(const UAction*)> Predicate);

	/** Cancel all actions with matching owner and predicate */
	void CancelByOwnerPredicate(UObject* Object, TFunctionRef<bool(const UAction*)> Predicate);

private:
	void AddRootAction(UAction* Child);
	void AddActionToTickGroup(UAction* Child);
	void RemoveActionFromTickGroup(UAction* Child);

public:
#if WITH_GAMEPLAY_DEBUGGER
	void DescribeOwnerToGameplayDebugger(
		UObject* Owner, const FName& BaseName, class FGameplayDebugger_Actions& Debugger) const;
#endif	  // WITH_GAMEPLAY_DEBUGGER


	FORCEINLINE static UActionsSubsystem* Get(UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return UGameInstance::GetSubsystem<UActionsSubsystem>(GI);
	}
};
