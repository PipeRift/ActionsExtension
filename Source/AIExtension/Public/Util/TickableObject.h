// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/NoExportTypes.h"
#include "TickableObject.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UTickableObject : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
    enum class EObjectBeginPlayState : uint8
    {
        HasNotBegunPlay,
        BeginningPlay,
        HasBegunPlay,
    };
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    bool bWantsToTick;

    //Tick length in seconds. 0 is default tick rate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    float TickRate;

protected:
    //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;

    virtual bool IsTickable() const override { return bWantsToTick; }
    virtual TStatId GetStatId() const override { return Super::GetStatID(); }
    //~ End FTickableGameObject Interface


    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "BeginPlay"))
    void ReceiveBeginPlay();

    /** Overridable native event for when play begins for this actor. */
    virtual void BeginPlay();


    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
    void ReceiveTick(float DeltaTime);

    virtual void Tick(float DeltaTime);

public:
    /** Initiate a begin play call on this Actor, will handle . */
    void DispatchBeginPlay();

    /** Returns whether an actor has been initialized */
    bool IsActorInitialized() const { return bActorInitialized; }

    /** Returns whether an actor is in the process of beginning play */
    bool IsActorBeginningPlay() const { return ActorHasBegunPlay == EActorBeginPlayState::BeginningPlay; }

    /** Returns whether an actor has had BeginPlay called on it (and not subsequently had EndPlay called) */
    bool HasActorBegunPlay() const { return ActorHasBegunPlay == EActorBeginPlayState::HasBegunPlay; }

	
};
