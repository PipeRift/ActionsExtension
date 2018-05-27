// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "ActionOwnerInterface.h"

#include "Action.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(TaskLog, Log, All);

/**
 * Result of a node execution
 */
UENUM(Blueprintable)
enum class EActionState : uint8
{
    RUNNING  UMETA(DisplayName = "Running"),
    SUCCESS  UMETA(DisplayName = "Success"),
    FAILURE  UMETA(DisplayName = "Failure"),
    ABORTED  UMETA(DisplayName = "Abort"),
    CANCELED UMETA(DisplayName = "Canceled"),
    NOT_RUN  UMETA(DisplayName = "Not Run")
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTaskActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTaskFinished, const EActionState, Reason);


class UActionManagerComponent;


/**
 * 
 */
UCLASS(Blueprintable, meta = (ExposedAsyncProxy))
class AIEXTENSION_API UAction : public UObject, public FTickableGameObject, public IActionOwnerInterface
{
    GENERATED_UCLASS_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = Action)
    void Activate();

    virtual const bool AddChildren(UAction* NewChildren) override;
    virtual const bool RemoveChildren(UAction* Children) override;


    /** Event when tick is received for this tickable object . */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
    void ReceiveTick(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = Action)
    void Finish(bool bSuccess = true);

    /** Called when any error occurs */
    UFUNCTION(BlueprintCallable, Category = Action)
    void Abort();

    /** Called when the task needs to be stopped from running */
    UFUNCTION(BlueprintCallable, Category = Action)
    void Cancel();

    void Destroy();

    UFUNCTION(BlueprintPure, Category = Action)
    virtual UActionManagerComponent* GetActionOwnerComponent() override;


    //~ Begin Ticking
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Action)
    uint8 bWantsToTick:1;

    //Tick length in seconds. 0 is default tick rate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Action)
    float TickRate;

private:

    float TickTimeElapsed;
    //~ End Ticking


protected:

    UPROPERTY()
    EActionState State;

    UPROPERTY()
    TArray<UAction*> ChildrenTasks;

    //~ Begin Tickable Object Interface
    virtual void Tick(float DeltaTime) override;
    virtual void TaskTick(float DeltaTime) {}

    virtual bool IsTickable() const override {
        return !IsDefaultSubobject() && bWantsToTick && IsRunning() && !GetParent()->IsPendingKill();
    }

    virtual TStatId GetStatId() const override {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UAction, STATGROUP_Tickables);
    }
    //~ End Tickable Object Interface

    inline virtual void OnActivation() {
        OnTaskActivation.Broadcast();
        ReceiveActivate();
    }

public:

    virtual void OnFinish(const EActionState Reason);

    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Activate"))
    void ReceiveActivate();

    /** Event when finishing this task. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finished"))
    void ReceiveFinished(const EActionState Reason);

    // DELEGATES
    UPROPERTY()
    FTaskActivated OnTaskActivation;

    UPROPERTY()
    FTaskFinished OnTaskFinished;


    // INLINES

    const bool IsValid() const {
        UObject const * const Outer = GetOuter();
        return !IsPendingKill() && Outer->Implements<UActionOwnerInterface>();
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
    FORCEINLINE bool IsRunning() const { return IsValid() && State == EActionState::RUNNING; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
    FORCEINLINE bool Succeeded() const { return IsValid() && State == EActionState::SUCCESS; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
    FORCEINLINE bool Failed() const    { return IsValid() && State == EActionState::FAILURE; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
    FORCEINLINE bool IsCanceled() const { return State == EActionState::CANCELED; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
    FORCEINLINE EActionState GetState() const { return State; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Action)
    FORCEINLINE UObject* const GetParent() const {
        return IsValid() ? GetOuter() : nullptr;
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
        const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETaskState"), true);
        if (!EnumPtr)
            return FString("Invalid");

        return EnumPtr->GetNameByValue((int64)Value).ToString();
    }
};
