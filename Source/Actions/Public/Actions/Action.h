// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <EngineMinimal.h>
#include <UObject/ObjectMacros.h>
#include <UObject/ScriptInterface.h>
#include <Tickable.h>

#include "ActionsModule.h"
#include "ActionOwnerInterface.h"
#include "Action.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(ActionLog, Log, All);

class UActionManagerComponent;


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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionActivatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionFinishedDelegate, const EActionState, Reason);


/**
 *
 */
UCLASS(Blueprintable, EditInlineNew, meta = (ExposedAsyncProxy))
class ACTIONS_API UAction : public UObject, public FTickableGameObject, public IActionOwnerInterface
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY()
	EActionState State;

	UPROPERTY()
	TArray<UAction*> ChildrenActions;

	//~ Begin Ticking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Action)
	bool bWantsToTick;

	//Tick length in seconds. 0 is default tick rate
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Action)
	float TickRate;

private:

	float TickTimeElapsed;
	//~ End Ticking

public:

	// DELEGATES

	UPROPERTY()
	FActionActivatedDelegate OnActivationDelegate;

	UPROPERTY()
	FActionFinishedDelegate OnFinishedDelegate;


	UFUNCTION(BlueprintCallable, Category = Action)
	void Activate();

	UFUNCTION(BlueprintCallable, Category = Action)
	void Succeed() { Finish(true); }

	UFUNCTION(BlueprintCallable, Category = Action, meta = (AdvancedDisplay = "Message"))
	void Fail(FName Error = NAME_None) {
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

	UFUNCTION(BlueprintPure, Category = Action)
	virtual UActionManagerComponent* GetActionOwnerComponent() const override;

protected:

	virtual bool CanActivate() { return EventCanActivate(); }
	virtual void OnActivation() {
		OnActivationDelegate.Broadcast();
		ReceiveActivate();
	}

	/** Event called when finishing this task. */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Can Activate"))
	bool EventCanActivate();

private:

	void Finish(bool bSuccess = true);

	void Destroy();

protected:

	//~ Begin ActorOwnerInterface
	virtual const bool AddChildren(UAction* NewChildren) override;
	virtual const bool RemoveChildren(UAction* Children) override;
	//~ End ActorOwnerInterface


	//~ Begin Tickable Object Interface
	virtual void Tick(float DeltaTime) override;
	virtual void ActionTick(float DeltaTime) {}

	virtual bool IsTickable() const override {
		return !IsDefaultSubobject() && bWantsToTick && IsRunning() && !GetParent()->IsPendingKill();
	}

	virtual TStatId GetStatId() const override {
		RETURN_QUICK_DECLARE_CYCLE_STAT(UAction, STATGROUP_Tickables);
	}
	//~ End Tickable Object Interface

public:

	// INLINES

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
	FORCEINLINE bool IsRunning() const { return State == EActionState::RUNNING; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
	FORCEINLINE bool Succeeded() const { return State == EActionState::SUCCESS; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
	FORCEINLINE bool Failed() const	{ return State == EActionState::FAILURE; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
	FORCEINLINE EActionState GetState() const { return State; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
	FORCEINLINE UObject* const GetParent() const {
		UObject* const Outer = GetOuter();
		return (Outer && Outer->Implements<UActionOwnerInterface>())? Outer : nullptr;
	}

	FORCEINLINE IActionOwnerInterface* GetParentInterface() const {
		return Cast<IActionOwnerInterface>(GetOuter());
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
	AActor* GetActionOwnerActor();

	virtual UWorld* GetWorld() const override {
		const UObject* const InParent = GetParent();
		return InParent ? InParent->GetWorld() : nullptr;
	}

	static FORCEINLINE FString StateToString(EActionState Value) {
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EActionState"), true);
		if (!EnumPtr)
			return FString("Invalid");
		return EnumPtr->GetNameByValue((int64)Value).ToString();
	}


	/** STATIC */

	template<typename ActionType>
	static ActionType* Create(const TScriptInterface<IActionOwnerInterface>& Owner, bool bAutoActivate = false) {
		return Cast<ActionType>(Create(Owner, ActionType::StaticClass(), bAutoActivate));
	}

	static UAction* Create(const TScriptInterface<IActionOwnerInterface>& Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate = false);
	static UAction* Create(const TScriptInterface<IActionOwnerInterface>& Owner, class UAction* Template, bool bAutoActivate = false);
};
