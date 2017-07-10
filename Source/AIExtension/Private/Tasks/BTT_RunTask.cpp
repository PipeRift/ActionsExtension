// Fill out your copyright notice in the Description page of Project Settings.

#include "AIExtension/Private/AIExtensionPrivatePCH.h"
#include "AIGeneric.h"
#include "TaskFunctionLibrary.h"

#include "BTT_RunTask.h"

UBTT_RunTask::UBTT_RunTask()
{
    TaskClass = UTask::StaticClass();
}

EBTNodeResult::Type UBTT_RunTask::ExecuteTask(UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory)
{
    if(!TaskClass.Get()) {
        return EBTNodeResult::Failed;
    }

    //TODO: Implement TaskRunner
    //UTaskFunctionLibrary::CreateTask()

    AActor* OwnerActor = InOwnerComp.GetTypedOuter<AActor>();
    check(OwnerActor);

    //Find Task interface
    if (OwnerActor->GetClass()->ImplementsInterface(UTaskOwnerInterface::StaticClass()))
    {
        TaskInterface.SetInterface(Cast<ITaskOwnerInterface>(OwnerActor));
        TaskInterface.SetObject(OwnerActor);
    }
    else
    {
        //Does Owner Actor have a Task Manager component?
        UTaskManagerComponent* const Manager = OwnerActor->FindComponentByClass<UTaskManagerComponent>();
        if (Manager)
        {
            TaskInterface.SetInterface(Cast<ITaskOwnerInterface>(Manager));
            TaskInterface.SetObject(Manager);
        }

    }

    if (!TaskInterface.GetObject())
    {
        return EBTNodeResult::Failed;
    }

    Task = UTaskFunctionLibrary::CreateTask(TaskInterface, TaskClass, true);
    check(Task);

    Task->OnTaskFinished.AddDynamic(this, &UBTT_RunTask::OnRunTaskFinished);

    OwnerComp = &InOwnerComp;
    return Task? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTT_RunTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if(Task)
    {
        Task->Cancel();
    }
    return EBTNodeResult::Aborted;
}

void UBTT_RunTask::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
    Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

    if (Task)
    {
        const FString State = UTask::StateToString(Task->GetState());
        Values.Add(FString::Printf(TEXT("state: %s"), *State));
    }
}

FString UBTT_RunTask::GetStaticDescription() const
{
    const FString TaskName = (*TaskClass)? TaskClass->GetName() : "None";
    return FString::Printf(TEXT("Task: %s"), *TaskName);
}


const bool UBTT_RunTask::AddChildren(UTask* NewChildren)
{
    return (*TaskInterface).AddChildren(NewChildren);
}

const bool UBTT_RunTask::RemoveChildren(UTask* Children)
{
    return (*TaskInterface).RemoveChildren(Children);
}

UTaskManagerComponent* UBTT_RunTask::GetTaskOwnerComponent()
{
    return (*TaskInterface).GetTaskOwnerComponent();
}

void UBTT_RunTask::OnRunTaskFinished(const ETaskState Reason)
{
    if (OwnerComp)
    {
        switch (Reason)
        {
        case ETaskState::SUCCESS:
            FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
            break;
        case ETaskState::FAILURE:
            FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
            break;
        case ETaskState::CANCELED: //Do Nothing
            break;
        default:
            FinishLatentTask(*OwnerComp, EBTNodeResult::Aborted);
            break;
        }
    }
}