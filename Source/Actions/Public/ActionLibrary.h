// Copyright 2015-2026 Piperift. All Rights Reserved.

#pragma once

#include "Action.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "ActionLibrary.generated.h"


UCLASS()
class ACTIONS_API UActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Action,
		meta = (BlueprintInternalUseOnly = "true", DefaultToSelf = "Owner", WorldContext = "Owner"))
	static UAction* CreateAction(UObject* Owner, const TSubclassOf<UAction> Class, bool bAutoActivate = false)
	{
		return ::CreateAction(Owner, Class.Get(), bAutoActivate);
	}
};
