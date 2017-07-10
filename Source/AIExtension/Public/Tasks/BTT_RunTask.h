// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"

#include "Task.h"
#include "TaskOwnerInterface.h"

#include "BTT_RunTask.generated.h"


/**
 * 
 */
UCLASS()
class AIEXTENSION_API UBTT_RunTask : public UBTTaskNode, public ITaskOwnerInterface
{
	GENERATED_BODY()	
	
public:
    UBTT_RunTask();


    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node, meta = (DisplayName = "Task"))
    TSubclassOf<UTask> TaskClass;

    UPROPERTY()
    TScriptInterface<ITaskOwnerInterface> TaskInterface;

    UPROPERTY()
    UTask* Task;

    UPROPERTY(Transient)
    UBehaviorTreeComponent* OwnerComp;


    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
    virtual FString GetStaticDescription() const override;


    // Begin ITaskOwnerInterface interface
    virtual const bool AddChildren(UTask* NewChildren) override;
    virtual const bool RemoveChildren(UTask* Children) override;
    virtual UTaskManagerComponent* GetTaskOwnerComponent() override;
    // End ITaskOwnerInterface interface

    UFUNCTION()
    void OnRunTaskFinished(const ETaskState Reason);
};
