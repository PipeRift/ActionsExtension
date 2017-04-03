// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/NoExportTypes.h"
#include "TickableObject.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class AIEXTENSION_API UTickableObject : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()
	
    enum class EObjectBeginPlayState : uint8
    {
        HasNotBegunPlay,
        BeginningPlay,
        HasBegunPlay,
    };

    /**
    *	Indicates that BeginPlay has been called for this Actor.
    *  Set back to false once EndPlay has been called.
    */
    EObjectBeginPlayState ObjectHasBegunPlay : 2;
    
    /** 
	 *	Indicates that PreInitializeComponents/PostInitializeComponents have been called on this Actor 
	 *	Prevents re-initializing of actors spawned during level startup
	 */
	uint8 bObjectInitialized:1;

    float DeltaElapsed;
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    bool bWantsToTick;

    //Can the object tick in editor?
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    bool bTickInEditor;

    //Tick length in seconds. 0 is default tick rate
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Object)
    float TickRate;

protected:
    //~ Begin UObject Interface
    /*virtual UWorld* GetWorld() override {
        return Outer ? Outer->GetWorld() : nullptr;
    }*/
    virtual void PostInitProperties() override;
    //~ End UObject Interface
    
    //~ Begin FTickableGameObject Interface
    virtual void Tick(float DeltaTime) override;

    virtual bool IsTickable() const override {
        //Don't tick in editor if we dont want to.
        const bool bIsEditorAndStarted = (GWorld->HasBegunPlay() && !GIsEditor);
        return bWantsToTick && bIsEditorAndStarted;
    }
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual TStatId GetStatId() const override {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UTickableObject, STATGROUP_Tickables);
    }
    //~ End FTickableGameObject Interface



    /** Overridable native event for when play begins for this tickable object . */
    virtual void BeginPlay();

    virtual void OTick(float DeltaTime);

public:
    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "BeginPlay"))
    void ReceiveBeginPlay();

    /** Event when tick is received for this tickable object . */
    UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Tick"))
    void ReceiveTick(float DeltaTime);


    /** Initiate a begin play call on this tickable object, will handle . */
    void DispatchBeginPlay();

    UFUNCTION(BlueprintCallable, Category = Object)
    void Destroy();


    /** Returns whether an tickable object  has been initialized */
    FORCEINLINE bool IsObjectInitialized() const { return bObjectInitialized; }

    /** Returns whether an tickable object is in the process of beginning play */
    FORCEINLINE bool IsObjectBeginningPlay() const { return ObjectHasBegunPlay == EObjectBeginPlayState::BeginningPlay; }

    /** Returns whether an tickable object  has had BeginPlay called on it (and not subsequently had EndPlay called) */
    FORCEINLINE bool HasObjectBegunPlay() const { return ObjectHasBegunPlay == EObjectBeginPlayState::HasBegunPlay; }

    virtual UWorld* GetWorld() const override {
        const UObject* Outer = GetOuter();
        return Outer ? Outer->GetWorld() : nullptr;
    }
};
