// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "ActionOwnerInterface.h"
#include "ActionManagerComponent.generated.h"

#if WITH_GAMEPLAY_DEBUGGER
class FGameplayDebugger_Actions;
#endif // WITH_GAMEPLAY_DEBUGGER


UCLASS(BlueprintType, ClassGroup = (Actions), meta = (BlueprintSpawnableComponent))
class ACTIONS_API UActionManagerComponent : public UActorComponent, public IActionOwnerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UActionManagerComponent();

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UFUNCTION(BlueprintCallable, Category = ActionsManager)
	void CancelAll();

	void CancelByPredicate(TFunctionRef<bool(const UAction*)> Predicate);


	// Begin ITaskOwnerInterface interface
	virtual const bool AddChildren(UAction* NewChildren) override;
	virtual const bool RemoveChildren(UAction* Children) override;
	virtual UActionManagerComponent* GetActionOwnerComponent() const override;
	// End ITaskOwnerInterface interface


#if WITH_GAMEPLAY_DEBUGGER
	void DescribeSelfToGameplayDebugger(const FName& BaseName, FGameplayDebugger_Actions& Debugger) const;
	void DescribeActionToGameplayDebugger(const UAction* Action, FGameplayDebugger_Actions& Debugger, int8 Indent) const;
#endif // WITH_GAMEPLAY_DEBUGGER

protected:

	UPROPERTY(SaveGame)
	TArray<UAction*> ChildrenActions;
};
