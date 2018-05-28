// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"

#include "Action.h"
#include "ActionOwnerInterface.h"

#include "BTT_RunAction.generated.h"


/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTT_RunAction : public UBTTaskNode, public IActionOwnerInterface
{
	GENERATED_BODY()	
	
public:
	UBTT_RunAction();


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node, meta = (DisplayName = "Action"))
	TSubclassOf<UAction> ActionClass;

	UPROPERTY()
	TScriptInterface<IActionOwnerInterface> ActionInterface;

	UPROPERTY()
	UAction* Action;

	UPROPERTY(Transient)
	UBehaviorTreeComponent* OwnerComp;


	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory) override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;


	// Begin ITaskOwnerInterface interface
	virtual const bool AddChildren(UAction* NewChildren) override;
	virtual const bool RemoveChildren(UAction* Children) override;
	virtual UActionManagerComponent* GetActionOwnerComponent() override;
	// End ITaskOwnerInterface interface

	UFUNCTION()
	void OnRunActionFinished(const EActionState Reason);
};
