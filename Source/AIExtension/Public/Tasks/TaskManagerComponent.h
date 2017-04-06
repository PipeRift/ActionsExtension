// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "TaskOwnerInterface.h"
#include "TaskManagerComponent.generated.h"


UCLASS(Blueprintable, ClassGroup = (Tasks), meta = (BlueprintSpawnableComponent))
class AIEXTENSION_API UTaskManagerComponent : public UActorComponent, public ITaskOwnerInterface
{
    GENERATED_BODY()

public:	
    // Sets default values for this component's properties
    UTaskManagerComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual const bool AddChildren(UTask* NewChildren) override;
    virtual const bool RemoveChildren(UTask* Children) override;


    UFUNCTION(BlueprintNativeEvent, Category = Task, meta = (BlueprintInternalUseOnly = "true"))
    UTaskManagerComponent* GetTaskOwnerComponent();

protected:
    TSet<TSharedPtr<UTask>> ChildrenTasks;
};
