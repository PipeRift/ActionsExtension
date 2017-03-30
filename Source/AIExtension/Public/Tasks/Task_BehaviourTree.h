// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BrainComponent.h"
#include "GameplayTaskOwnerInterface.h"
#include "Task.h"
#include "Task_BehaviourTree.generated.h"

/**
* Result of a node execution
*/
UENUM()
enum class EBTState : uint8
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
UCLASS(Blueprintable)
class AIEXTENSION_API UTask_BehaviourTree : public UTask
{
    GENERATED_BODY()

public:
    UPROPERTY()
    EBTState BTState;

    UTask_BehaviourTree(const FObjectInitializer& ObjectInitializer);

    virtual void OnActivation() override;
    virtual void TaskTick(float DeltaTime) override;

    UFUNCTION(BlueprintImplementableEvent, Category = BehaviourTree)
    void Root();

    UFUNCTION(BlueprintCallable, Category = BehaviourTree)
    void Success();
    UFUNCTION(BlueprintCallable, Category = BehaviourTree)
    void Failure(bool bError);

private:
    void SetState(EBTState NewState);

    UPROPERTY()
    UTask* LastExecutedNode;

public:
    //Inlines
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = Task)
    FORCEINLINE bool IsRunning() { return BTState == EBTState::RUNNING; }
};
