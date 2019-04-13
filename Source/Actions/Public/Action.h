// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <EngineMinimal.h>
#include <UObject/ObjectMacros.h>
#include <UObject/ScriptInterface.h>
#include <Tickable.h>

#include "ActionsModule.h"
#include "ActionsSubsystem.h"
#include "Action.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(ActionLog, Log, All);


/**
 * Result of a node execution
 */
UENUM(Blueprintable)
enum class EActionState : uint8
{
	PREPARING UMETA(DisplayName = "Preparing", Hidden),
	RUNNING   UMETA(DisplayName = "Running", Hidden),
	SUCCESS   UMETA(DisplayName = "Success"),
	FAILURE   UMETA(DisplayName = "Failure"),
	CANCELED  UMETA(DisplayName = "Canceled")
};

FORCEINLINE FString ToString(EActionState Value)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EActionState"), true);
	return EnumPtr? EnumPtr->GetNameByValue((int64)Value).ToString() : TEXT("Invalid");
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionActivatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionFinishedDelegate, const EActionState, Reason);


/**
 *
 */
UCLASS(Blueprintable, EditInlineNew, meta = (ExposedAsyncProxy))
class ACTIONS_API UAction : public UObject
{
	GENERATED_UCLASS_BODY()

	friend UAction;

public:

	UPROPERTY(SaveGame)
	EActionState State;

	UPROPERTY(SaveGame)
	TSet<UAction*> ChildrenActions;

	//~ Begin Ticking
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Action)
	bool bWantsToTick;

protected:

	//Tick length in seconds. 0 is default tick rate
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Action, BlueprintGetter="GetTickRate")
	float TickRate;
	//~ End Ticking

public:

	// DELEGATES

	UPROPERTY()
	FActionActivatedDelegate OnActivationDelegate;

	UPROPERTY()
	FActionFinishedDelegate OnFinishedDelegate;


	UFUNCTION(BlueprintCallable, Category = Action)
	void Activate();

	UFUNCTION(BlueprintCallable, Category = Action, meta = (KeyWords = "Finish"))
	void Succeed() { Finish(true); }

	UFUNCTION(BlueprintCallable, Category = Action, meta = (KeyWords = "Finish"))
	void Fail(FName Error = NAME_None)
	{
		UE_LOG(LogActions, Log, TEXT("Action '%s' failed: %s"), *GetName(), *Error.ToString());
		Finish(false);
	}

	/** Event called when play begins for this actor. */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Activate"))
	void ReceiveActivate();

	/** Event called when tick is received for this tickable object . */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
	void ReceiveTick(float DeltaTime);

	/** Event called when finishing this task. */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finished"))
	void ReceiveFinished(const EActionState Reason);

	/** Used Internally. Called when the task is stopped from running by its owner */
	void Cancel();

	virtual void OnFinish(const EActionState Reason);


	void DoTick(float DeltaTime)
	{
		Tick(DeltaTime);
		ReceiveTick(DeltaTime);
	}

protected:

	virtual bool CanActivate() { return EventCanActivate(); }
	virtual void OnActivation() {
		OnActivationDelegate.Broadcast();
		ReceiveActivate();
	}

	/** Event called when finishing this task. */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Can Activate"))
	bool EventCanActivate();

	virtual void Tick(float DeltaTime) {}

private:

	void Finish(bool bSuccess = true);

	void Destroy();

	void AddChildren(UAction* Child);
	void RemoveChildren(UAction* Child);


public:

	// INLINES

	FORCEINLINE bool CanTick() const
	{
		return bWantsToTick && IsRunning() && !GetOuter()->IsPendingKill();
	}

	UFUNCTION(BlueprintPure, Category = Action)
	FORCEINLINE bool IsRunning() const { return !IsPendingKill() && State == EActionState::RUNNING; }

	UFUNCTION(BlueprintPure, Category = Action)
	FORCEINLINE bool Succeeded() const { return !IsPendingKill() && State == EActionState::SUCCESS; }

	UFUNCTION(BlueprintPure, Category = Action)
	FORCEINLINE bool Failed()    const { return !IsPendingKill() && State == EActionState::FAILURE; }

	UFUNCTION(BlueprintPure, Category = Action)
	FORCEINLINE EActionState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = Action)
	FORCEINLINE UObject* const GetParent() const { return GetOuter(); }

	UFUNCTION(BlueprintPure, Category = Action)
	FORCEINLINE UAction* GetParentAction() const { return Cast<UAction>(GetOuter()); }

	UFUNCTION(BlueprintGetter)
	FORCEINLINE float GetTickRate() const
	{
		return FMath::FloorToFloat(TickRate * 1000.f) * 0.001f;
	}

	/** @return the object that executes the root action */
	UFUNCTION(BlueprintPure, Category = Action)
	UObject* GetOwner() const;

	/** @return the actor if any that executes the root action */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = Action)
	AActor* GetOwnerActor() const;

	/** @return the component if any that executes the root action */
	UFUNCTION(BlueprintPure, Category = Action)
	UActorComponent* GetOwnerComponent() const { return Cast<UActorComponent>(GetOwner()); }

	virtual UWorld* GetWorld() const override
	{
		// If we are a CDO, we must return nullptr to fool UObject::ImplementsGetWorld.
		if (HasAllFlags(RF_ClassDefaultObject))
			return nullptr;

		const UObject* const InOwner = GetOwner();
		return InOwner ? InOwner->GetWorld() : nullptr;
	}


private:

	UActionsSubsystem* GetSubsystem() const {
		UWorld* World = GetWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return UGameInstance::GetSubsystem<UActionsSubsystem>(GI);
	}

public:

	/** STATIC */

	template<typename ActionType>
	static ActionType* Create(UObject* Owner, bool bAutoActivate = false) {
		return Cast<ActionType>(Create(Owner, ActionType::StaticClass(), bAutoActivate));
	}

	static UAction* Create(UObject* Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate = false);
	static UAction* Create(UObject* Owner, class UAction* Template, bool bAutoActivate = false);
};
