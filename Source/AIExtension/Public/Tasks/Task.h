// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "TaskOwnerInterface.h"

#include "Task.generated.h"

/**
 * Result of a node execution
 */
UENUM()
enum class ETaskState : uint8
{
    RUNNING  UMETA(DisplayName = "Running"),
    SUCCESS  UMETA(DisplayName = "Success"),
    FAILURE  UMETA(DisplayName = "Failure"),
    ERROR    UMETA(DisplayName = "Error"),
    CANCELED UMETA(DisplayName = "Canceled"),
    NOT_RUN  UMETA(DisplayName = "Not Run")
};

/**
 * 
 */
UCLASS(Blueprintable, meta = (ExposedAsyncProxy))
class AIEXTENSION_API UTask : public UObject, public FTickableGameObject, public ITaskOwnerInterface
{
    GENERATED_UCLASS_BODY()

public:
    void Initialize(ITaskOwnerInterface* InTaskOwner);

    UFUNCTION(BlueprintCallable, Category = Task)
    void Activate();

    virtual void AddChildren(UTask* NewChildren) override;
    virtual void RemoveChildren(UTask* Children) override;

    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, meta = (DisplayName = "Activate"))
    void ReceiveActivate();

    /** Event when tick is received for this tickable object . */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
    void ReceiveTick(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = Task)
    void FinishTask(bool bSuccess, bool bError);
    UFUNCTION(BlueprintCallable, Category = Task)
    void Cancel();

protected:

    void Destroy();

    virtual void OnActivation() {}

    /** Event when finishing this task. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finished"))
    void ReceiveFinished(bool bSucceded);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Task)
    UTaskComponent* GetTaskOwnerComponent();

public:
    //~ Begin Ticking
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    bool bWantsToTick;

    //Tick length in seconds. 0 is default tick rate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    float TickRate;

private:
    float TickTimeElapsed;
    //~ End Ticking

protected:
    ETaskState State;

    TWeakObjectPtr<UObject> Owner;
    TSet<TSharedPtr<UTask>> ChildrenTasks;

    //~ Begin Tickable Object Interface
    virtual void Tick(float DeltaTime) override;
    virtual void TaskTick(float DeltaTime) {}

    virtual bool IsTickable() const override {
        return bWantsToTick && IsActivated()
            && !IsPendingKill()
            && Owner.IsValid() && !Owner->IsPendingKill();
    }

    virtual TStatId GetStatId() const override {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UTask, STATGROUP_Tickables);
    }
    //~ End Tickable Object Interface

public:
    //Inlines
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool IsInitialized() const { return Owner.IsValid(); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool IsActivated() const { return IsInitialized() && State == ETaskState::RUNNING; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool Succeeded() const { return IsInitialized() && State == ETaskState::SUCCESS; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool Failed() const { return IsInitialized() && State == ETaskState::FAILURE; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE ETaskState GetState() const { return State; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    UObject* GetOwner() const { return Owner.IsValid() ? Owner.Get() : nullptr; }


    virtual UWorld* GetWorld() const override {
        const UObject* InOwner = GetOwner();
        return InOwner ? InOwner->GetWorld() : nullptr;
    }
};
