// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "Action.h"
#include "ActionLibrary.generated.h"


UCLASS()
class ACTIONS_API UActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = Action, meta = (DisplayName = "Create Action", BlueprintInternalUseOnly = "true", DefaultToSelf = "Owner", WorldContext = "Owner"))
	static UAction* CreateAction(UObject* Owner, const TSubclassOf<UAction> Type, bool bAutoActivate = false)
	{
		return UAction::Create(Owner, Type.Get(), bAutoActivate);
	}
};
