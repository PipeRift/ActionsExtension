// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"

#include "AISquad.h"
#include "Squad.generated.h"


UCLASS(BlueprintType)
class AIEXTENSION_API ASquad : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASquad();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
    FName Name;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
    FColor Colour;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector PositionIncrement;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AAISquad* SquadAI;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UTextRenderComponent* TextRender;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    USphereComponent* ObjectiveDirection;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    virtual void OnConstruction(const FTransform& Transform) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    
    UFUNCTION(BlueprintCallable)
    AAISquad* GetAI();
};
