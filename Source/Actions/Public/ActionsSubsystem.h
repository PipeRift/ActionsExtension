// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Subsystems/GameInstanceSubsystem.h>
#include <Tickable.h>

#include "ActionsSubsystem.generated.h"


class UAction;


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

	FORCEINLINE bool operator==(const FActionsTickGroup& Other) const { return FMath::IsNearlyEqual(TickRate, Other.TickRate); }
	FORCEINLINE bool operator!=(const FActionsTickGroup& Other) const { return !(*this == Other); }

	friend const uint32 GetTypeHash(const FActionsTickGroup& InGroup) { return GetTypeHash(InGroup.TickRate); }
};

USTRUCT()
struct FRootAction
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TArray<UAction*> Actions;


	FRootAction(const UObject* Owner = nullptr) : Owner(Owner) {}

	void CancelAll(bool bShouldShrink = true);
	void CancelByPredicate(const TFunctionRef<bool(const UAction*)>& Predicate, bool bShouldShrink = true);

	/**
	 * Operator overloading & Hashes
	 */
	FORCEINLINE bool operator==(const FRootAction& Other) const { return Owner == Other.Owner; }
	FORCEINLINE bool operator!=(const FRootAction& Other) const { return !(*this == Other); }

	friend uint32 GetTypeHash(const FRootAction& InAction) { return GetTypeHash(InAction.Owner); }
};

UCLASS()
class ACTIONS_API UActionsSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

	friend UAction;

private:

	UPROPERTY(SaveGame)
	TSet<FRootAction> RootActions;

	UPROPERTY(Transient)
	TArray<FActionsTickGroup> TickGroups;


protected:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:

	//~ Begin Tickable GameObject Interface
	virtual void Tick(float DeltaTime) override;

	virtual bool IsTickable() const override {
		return RootActions.Num() > 0 || TickGroups.Num() > 0;
	}

	virtual TStatId GetStatId() const override {
		RETURN_QUICK_DECLARE_CYCLE_STAT(UActionsSubsystem, STATGROUP_Tickables);
	}
	//~ End Tickable GameObject Interface


	void CancelAll();

	void CancelAllByObject(UObject* Object);

	void CancelByPredicate(TFunctionRef<bool(const UAction*)> Predicate);
	void CancelByObjectPredicate(UObject* Object, TFunctionRef<bool(const UAction*)> Predicate);

private:

	void AddRootAction(UAction* Child);
	void AddActionToTickGroup(UAction* Child);

public:

#if WITH_GAMEPLAY_DEBUGGER
	void DescribeObjectToGameplayDebugger(const UObject* Object, const FName& BaseName, class FGameplayDebugger_Actions& Debugger) const;
#endif // WITH_GAMEPLAY_DEBUGGER


	FORCEINLINE static UActionsSubsystem* Get(UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return UGameInstance::GetSubsystem<UActionsSubsystem>(GI);
	}
};
