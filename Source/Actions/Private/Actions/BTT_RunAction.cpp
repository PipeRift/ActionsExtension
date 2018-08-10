// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "BTT_RunAction.h"

#include "ActionFunctionLibrary.h"
#include "ActionManagerComponent.h"


UBTT_RunAction::UBTT_RunAction()
{
	ActionClass = UAction::StaticClass();
}

EBTNodeResult::Type UBTT_RunAction::ExecuteTask(UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory)
{
	if(!ActionClass.Get()) {
		return EBTNodeResult::Failed;
	}

	//TODO: Implement TaskRunner
	//UTaskFunctionLibrary::CreateTask()

	AActor* OwnerActor = InOwnerComp.GetTypedOuter<AActor>();
	check(OwnerActor);

	//Find Task interface
	if (OwnerActor->GetClass()->ImplementsInterface(UActionOwnerInterface::StaticClass()))
	{
		ActionInterface.SetInterface(Cast<IActionOwnerInterface>(OwnerActor));
		ActionInterface.SetObject(OwnerActor);
	}
	else
	{
		//Does Owner Actor have a Task Manager component?
		UActionManagerComponent* const Manager = OwnerActor->FindComponentByClass<UActionManagerComponent>();
		if (Manager)
		{
			ActionInterface.SetInterface(Cast<IActionOwnerInterface>(Manager));
			ActionInterface.SetObject(Manager);
		}

	}

	if (!ActionInterface.GetObject())
	{
		return EBTNodeResult::Failed;
	}

	Action = UActionFunctionLibrary::CreateAction(ActionInterface, ActionClass, true);
	check(Action);

	Action->OnTaskFinished.AddDynamic(this, &UBTT_RunAction::OnRunActionFinished);

	OwnerComp = &InOwnerComp;
	return Action? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTT_RunAction::AbortTask(UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory)
{
	if(Action)
	{
		Action->Cancel();
	}
	return EBTNodeResult::Aborted;
}

void UBTT_RunAction::DescribeRuntimeValues(const UBehaviorTreeComponent& InOwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(InOwnerComp, NodeMemory, Verbosity, Values);

	if (Action)
	{
		const FString State = UAction::StateToString(Action->GetState());
		Values.Add(FString::Printf(TEXT("state: %s"), *State));
	}
}

FString UBTT_RunAction::GetStaticDescription() const
{
	const FString ActionName = (*ActionClass)? ActionClass->GetName() : "None";
	return FString::Printf(TEXT("Action: %s"), *ActionName);
}


const bool UBTT_RunAction::AddChildren(UAction* NewChildren)
{
	return (*ActionInterface).AddChildren(NewChildren);
}

const bool UBTT_RunAction::RemoveChildren(UAction* Children)
{
	return (*ActionInterface).RemoveChildren(Children);
}

UActionManagerComponent* UBTT_RunAction::GetActionOwnerComponent() const
{
	return ActionInterface->GetActionOwnerComponent();
}

void UBTT_RunAction::OnRunActionFinished(const EActionState Reason)
{
	if (OwnerComp)
	{
		switch (Reason)
		{
		case EActionState::SUCCESS:
			FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
			break;
		case EActionState::FAILURE:
			FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
			break;
		case EActionState::CANCELED: //Do Nothing
			break;
		default:
			FinishLatentTask(*OwnerComp, EBTNodeResult::Aborted);
			break;
		}
	}
}