// Copyright 2015-2023 Piperift. All Rights Reserved.

#pragma once

#include "ActionsModule.h"
#include "ActionsSubsystem.h"

#include <CoreMinimal.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <Tickable.h>
#include <UObject/ObjectMacros.h>
#include <UObject/ScriptInterface.h>

#include "Action.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(ActionLog, Log, All);


class AActor;
class UActorComponent;
class UAction;


/**
 * Creates a new action. Templated version
 * @param ActionType
 * @param Owner of the action. If destroyed, the action will follow.
 * @param bAutoActivate if true activates the action. If false, Action->Activate() can be called later.
 */
template <typename ActionType>
ActionType* CreateAction(UObject* Owner, bool bAutoActivate = false)
{
	static_assert(
		!std::is_same_v<ActionType, UAction>, "Instantiating UAction is not allowed. Use a child class.");
	static_assert(TIsDerivedFrom<ActionType, UAction>::IsDerived, "Provided class must inherit UAction.");
	return Cast<ActionType>(CreateAction(Owner, ActionType::StaticClass(), bAutoActivate));
}

/**
 * Creates a new action. Templated version
 * @param ActionType
 * @param Owner of the action. If destroyed, the action will follow.
 * @param Template whose properties and class are used to create the action.
 * @param bAutoActivate if true activates the action. If false, Action->Activate() can be called later.
 */
template <typename ActionType>
ActionType* CreateAction(UObject* Owner, const ActionType* Template, bool bAutoActivate = false)
	requires(!std::is_same_v<ActionType, UAction> && TIsDerivedFrom<ActionType, UAction>::IsDerived)
{
	return Cast<ActionType>(CreateAction(Owner, (const UAction*) Template, bAutoActivate));
}

/**
 * Creates a new action
 * @param Owner of the action. If destroyed, the action will follow.
 * @param Type of the action to create
 * @param bAutoActivate if true activates the action. If false, Action->Activate() can be called later.
 */
ACTIONS_API UAction* CreateAction(
	UObject* Owner, const TSubclassOf<UAction> Type, bool bAutoActivate = false);

/**
 * Creates a new action
 * @param Owner of the action. If destroyed, the action will follow.
 * @param Template whose properties and class are used to create the action.
 * @param bAutoActivate if true activates the action. If false, Action->Activate() can be called later.
 */
ACTIONS_API UAction* CreateAction(UObject* Owner, const UAction* Template, bool bAutoActivate = false);


/**
 * Result of a node execution
 */
UENUM(Blueprintable)
enum class EActionState : uint8
{
	Preparing UMETA(Hidden),
	Running UMETA(Hidden),
	Success,
	Failure,
	Cancelled
};

inline FString ToString(EActionState Value)
{
	const UEnum* EnumPtr =
		FindObject<UEnum>(nullptr, TEXT("/Script/Actions.EActionState"), EFindObjectFlags::ExactClass);
	return EnumPtr ? EnumPtr->GetNameByValue((int64) Value).ToString() : TEXT("Invalid");
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionActivatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionFinishedDelegate, const EActionState, Reason);


/**
 *
 */
UCLASS(Blueprintable, EditInlineNew, meta = (ExposedAsyncProxy))
class ACTIONS_API UAction : public UObject
{
	GENERATED_BODY()

	/************************************************************************/
	/* PROPERTIES														    */
	/************************************************************************/
private:
	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	EActionState State = EActionState::Preparing;

	UPROPERTY(SaveGame)
	TArray<UAction*> ChildrenActions;


	/** If true the action will tick. Tick can be enabled or disabled while running. */
	UPROPERTY(EditAnywhere, Category = Action)
	bool bWantsToTick = false;

protected:
	// Tick length in seconds. 0 is default tick rate
	UPROPERTY(EditDefaultsOnly, Category = Action)
	float TickRate = 0.15f;


public:
	/** Delegates */

	// Notify when the action is activated
	UPROPERTY()
	FActionActivatedDelegate OnActivationDelegate;

	// Notify when the action finished
	UPROPERTY()
	FActionFinishedDelegate OnFinishedDelegate;


	/************************************************************************/
	/* METHODS											     			    */
	/************************************************************************/

	/** Called to active an action if not already. */
	UFUNCTION(BlueprintCallable, Category = Action)
	bool Activate();

	/** Internal Use Only. Called when the action is stopped from running by its owner */
	void Cancel();

	/** Internal Use Only. Called by the subsystem when TickRate exceeds */
	void DoTick(float DeltaTime)
	{
		Tick(DeltaTime);
		ReceiveTick(DeltaTime);
	}

protected:
	UFUNCTION(BlueprintPure, Category = Action)
	virtual bool CanActivate()
	{
		return ReceiveCanActivate();
	}

	virtual void OnActivation()
	{
		OnActivationDelegate.Broadcast();
		ReceiveActivate();
	}

	virtual void Tick(float DeltaTime) {}

	virtual void OnFinish(const EActionState Reason);

private:
	void Finish(bool bSuccess = true);

	void Destroy();

	void AddChildren(UAction* Child);
	void RemoveChildren(UAction* Child);


public:
	UFUNCTION(BlueprintCallable, Category = Action, meta = (KeyWords = "Finish"))
	void Succeed()
	{
		Finish(true);
	}

	UFUNCTION(BlueprintCallable, Category = Action, meta = (KeyWords = "Finish"))
	void Fail(FName Error = NAME_None)
	{
		UE_LOG(LogActions, Log, TEXT("Action '%s' failed: %s"), *GetName(), *Error.ToString());
		Finish(false);
	}


	/** Events */
protected:
	/** Event called to check if an action can activate. */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Can Activate"))
	bool ReceiveCanActivate();

	/** Called when this action is activated */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Activate"))
	void ReceiveActivate();

	/** Called when tick is received based on TickRate */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
	void ReceiveTick(float DeltaTime);

	/** Called when this action finishes */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finished"))
	void ReceiveFinished(const EActionState Reason);


public:
	bool CanTick() const
	{
		return bWantsToTick && IsRunning();
	}

	UFUNCTION(BlueprintCallable, Category = Action)
	void SetWantsToTick(bool bValue);

	UFUNCTION(BlueprintPure, Category = Action)
	bool GetWantsToTick() const;

	UFUNCTION(BlueprintPure, Category = Action)
	float GetTickRate() const;

	UFUNCTION(BlueprintPure, Category = Action)
	bool IsRunning() const;

	UFUNCTION(BlueprintPure, Category = Action)
	bool Succeeded() const;

	UFUNCTION(BlueprintPure, Category = Action)
	bool Failed() const;

	UFUNCTION(BlueprintPure, Category = Action)
	EActionState GetState() const;

	UFUNCTION(BlueprintPure, Category = Action)
	UObject* const GetParent() const;

	UFUNCTION(BlueprintPure, Category = Action)
	UAction* GetParentAction() const;

	/** @return the object that executes the root action */
	UFUNCTION(BlueprintPure, Category = Action)
	UObject* GetOwner() const;

	/** @return the actor if any that executes the root action */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = Action)
	AActor* GetOwnerActor() const;

	/** @return the component if any that executes the root action */
	UFUNCTION(BlueprintPure, Category = Action)
	UActorComponent* GetOwnerComponent() const;

	UWorld* GetWorld() const override;

#if WITH_GAMEPLAY_DEBUGGER
	void DescribeSelfToGameplayDebugger(class FGameplayDebugger_Actions& Debugger, int8 Indent) const;
#endif	  // WITH_GAMEPLAY_DEBUGGER

	//~ Begin UObject Interface
	void PostInitProperties() override;
	//~ End UObject Interface

protected:
	UActionsSubsystem* GetSubsystem() const;
};
