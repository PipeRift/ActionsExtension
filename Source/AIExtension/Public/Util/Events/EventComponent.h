// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "EventHandler.h"
#include "EventComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEventExecuteSignature);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AIEXTENSION_API UEventComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    // Sets default values for this component's properties
    UEventComponent();
    
    //The default length in seconds that will be used for the timer.
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Length"), Category = "Event")
    float DefaultLength;
    //The default length in seconds that will be used for the timer.
    UPROPERTY(EditAnywhere, Category = "Event")
    bool bLooping;

    // Handle that manages the timer
    FEventHandler EventHandler;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type reason) override;

    /** Start the event timer.
    * Won't do anything if it's already running.
    * @param Length Set the event duration (Optional). By default the event Lenght will be used.
    */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Start"), Category = "Event")
    void StartTimespan(FTimespan Timespan) {
        Start((float)Timespan.GetMilliseconds() / 1000.0f);
    }
    /** Start the event timer. 
    * Won't do anything if it's already running.
    * @param Length Set the event duration (Optional). By default the event Lenght will be used.
    */
    UFUNCTION(BlueprintCallable, Category = "Event")
    bool Start(float Length = -1.0f);

    /** Pause the event timer.
    */
    UFUNCTION(BlueprintCallable, Category = "Event")
    void Pause();

    /** Resume the event timer.
    */
    UFUNCTION(BlueprintCallable, Category = "Event")
    void Resume();

    /** Reset the event and start it again from the start.
    * @param Length Set the event duration (Optional). By default the last length will be used.
    */
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Restart"), Category = "Event")
    void RestartTimespan(FTimespan Timespan) {
        Restart((float)Timespan.GetMilliseconds() / 1000.0f);
    }
    /** Reset the event and start it again from the start.
    * @param Length Set the event duration (Optional). By default the last length will be used.
    */
    UFUNCTION(BlueprintCallable, Category = "Event")
    void Restart(float Length = -1.0f);

    /** Stop and clean the event.
    */
    UFUNCTION(BlueprintCallable, Category = "Event")
    void Reset();

    /** Executed when the timer is done.
    */
    UFUNCTION()
    void OnExecute(int Id);

    // HELPERS
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event")
    bool IsRunning();
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event")
    bool IsPaused();
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Event")
    float GetLength();

    UPROPERTY(BlueprintAssignable, Category = "Event")
    FEventExecuteSignature Execute;
};
