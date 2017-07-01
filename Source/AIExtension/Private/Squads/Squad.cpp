// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Squad.h"


// Sets default values
ASquad::ASquad()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
    TextRender->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

    ObjectiveDirection = CreateDefaultSubobject<USphereComponent>(TEXT("ObjectiveDirection"));
    ObjectiveDirection->AttachToComponent(TextRender, FAttachmentTransformRules::KeepRelativeTransform);
    ObjectiveDirection->SetSphereRadius(100, true);
}

void ASquad::OnConstruction(const FTransform& Transform)
{
    TextRender->SetText(FText::FromName(Name));
    TextRender->SetTextRenderColor(Colour);
}

// Called when the game starts or when spawned
void ASquad::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASquad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASquad::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

AAISquad* ASquad::GetAI()
{
    if (!IsValid(SquadAI))
    {
        SquadAI = Cast<AAISquad>(UAIBlueprintHelperLibrary::GetAIController(this));
    }

    return SquadAI;
}