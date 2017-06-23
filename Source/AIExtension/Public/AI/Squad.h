// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI_Squad.h"
#include "Runtime/Engine/Classes/Components/TextRenderComponent.h"
#include "Runtime/Engine/Classes/Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "Squad.generated.h"

UCLASS()
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
    class AAI_Squad* SquadAI;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class UTextRenderComponent* TextRender;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    class USphereComponent* ObjectiveDirection;

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
    AAI_Squad* GetSquad();
};
