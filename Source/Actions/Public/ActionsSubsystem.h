// Copyright 2015-2026 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/World.h>
#include <Subsystems/WorldSubsystem.h>
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
class ACTIONSEXTENSION_API UActionsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	friend UAction;

private:
	UPROPERTY(SaveGame)
	TSet<FActionOwner> ActionOwners;

	UPROPERTY(Transient)
	TArray<FActionsTickGroup> TickGroups;


protected:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

public:
	void Tick(float DeltaTime) override;

	TStatId GetStatId() const override;


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
		return World ? World->GetSubsystem<UActionsSubsystem>() : nullptr;
	}
};
