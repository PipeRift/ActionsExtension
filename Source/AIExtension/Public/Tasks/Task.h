// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include "GameplayTask.h"

#include "TickableObject.h"

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
    NOT_RUN  UMETA(DisplayName = "Not Run")
};

/**
 * 
 */
UCLASS(Blueprintable, meta = (ExposedAsyncProxy))
class AIEXTENSION_API UTask : public UTickableObject, public IGameplayTaskOwnerInterface
{
    GENERATED_UCLASS_BODY()

protected:
    //~ Begin UTickableObject Interface
    /** Overridable native event for when play begins for this actor. */
    virtual void BeginPlay();
    virtual void OTick(float DeltaTime) override;

    virtual bool IsTickable() const override {
        return Super::IsTickable() && IsActivated();
    }

    //~ End UTickableObject Interface

public:
    UFUNCTION(BlueprintCallable, Category = Task)
    void Activate();

    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Activate"))
    void ReceiveActivate();

protected:
    virtual void OnActivation() {}


    UFUNCTION(BlueprintCallable, Category = Task)
    void FinishTask(bool bSuccess, bool bError);

    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finished"))
    void ReceiveFinished(bool bSucceded);

protected:
    UPROPERTY()
    ETaskState State;

public:
    //Inlines
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool IsActivated() const { return State == ETaskState::RUNNING; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool Succeeded() const { return State == ETaskState::SUCCESS; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool Failed() const { return State == ETaskState::FAILURE; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE ETaskState GetState() const { return State; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE UObject* GetOwner() { return GetOuter(); }
};
