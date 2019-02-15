// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "GameplayDebugger_Actions.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Actions/ActionManagerComponent.h"


#if WITH_GAMEPLAY_DEBUGGER

FGameplayDebugger_Actions::FGameplayDebugger_Actions()
{}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebugger_Actions::MakeInstance()
{
	return MakeShareable(new FGameplayDebugger_Actions());
}

void FGameplayDebugger_Actions::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	const UActionManagerComponent* PlayerActions = OwnerPC->FindComponentByClass<UActionManagerComponent>();
	if (PlayerActions)
	{
		PlayerActions->DescribeSelfToGameplayDebugger({ "Player Controller" }, *this);
	}

	if (DebugActor)
	{
		const UActionManagerComponent* SelectedActions = DebugActor->FindComponentByClass<UActionManagerComponent>();
		if (SelectedActions)
		{
			SelectedActions->DescribeSelfToGameplayDebugger({ "Actor" }, *this);
		}

		if (const APawn* DebugPawn = Cast<APawn>(DebugActor))
		{
			if (const AController* DebugController = DebugPawn->GetController())
			{
				const UActionManagerComponent* SelectedControllerActions = DebugController->FindComponentByClass<UActionManagerComponent>();
				if (PlayerActions != SelectedControllerActions)
				{
					SelectedControllerActions->DescribeSelfToGameplayDebugger({ "Controller" }, *this);
				}
			}
		}
	}
}
#endif // ENABLE_GAMEPLAY_DEBUGGER
