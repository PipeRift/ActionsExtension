// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "GameplayTask.h"

#include "Task.generated.h"

/**
 * Result of a node execution
 */
UENUM()
enum class ETaskState : uint8
{
    RUNNING UMETA(DisplayName = "Running"),
    SUCCESS UMETA(DisplayName = "Success"),
    FAILURE UMETA(DisplayName = "Failure"),
    ERROR   UMETA(DisplayName = "Error"),
    NOT_RUN  UMETA(DisplayName = "Not Run")
};

/**
 * 
 */
UCLASS(Blueprintable, meta = (ExposedAsyncProxy))
class AIEXTENSION_API UTask : public UObject, public FTickableGameObject, public IGameplayTaskOwnerInterface
{
    GENERATED_UCLASS_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishedDelegate);

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Task)
    bool bWantsToTick;
    //Tick length in seconds. 0 is default tick rate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Task)
    float TickRate;


    //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;
    virtual void TaskTick(float DeltaTime);
    virtual bool IsTickable() const override { return bWantsToTick; }
    virtual TStatId GetStatId() const override { return Super::GetStatID(); }
    //~ End FTickableGameObject Interface


    UFUNCTION(BlueprintCallable, Category = Task)
    void Activate();

    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Activate"))
    void ReceiveActivate();
    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
    void ReceiveTick(float DeltaTime);

protected:
    virtual void OnActivation() {}


    UFUNCTION(BlueprintCallable, Category = Task)
    void FinishTask(bool bSuccess, bool bError = false);
public:
    UPROPERTY(BlueprintAssignable, Category = Task)
    FOnFinishedDelegate OnFinished;


protected:
    UPROPERTY()
    ETaskState State;
private:
    float Elapsed;


public:
    //Inlines
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool IsActivated() { return State == ETaskState::RUNNING; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE ETaskState GetState() { return State; }
};
