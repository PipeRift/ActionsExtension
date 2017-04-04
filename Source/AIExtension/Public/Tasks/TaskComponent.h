// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "TaskOwnerInterface.h"
#include "TaskComponent.generated.h"


UCLASS( BlueprintType, meta=(BlueprintSpawnableComponent) )
class AIEXTENSION_API UTaskComponent : public UActorComponent, public ITaskOwnerInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTaskComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Task)
    UTaskComponent* GetTaskOwnerComponent();
};
