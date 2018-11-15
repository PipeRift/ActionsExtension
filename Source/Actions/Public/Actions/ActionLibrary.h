// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Action.h"
#include "ActionLibrary.generated.h"

/**
 *
 */
UCLASS()
class ACTIONS_API UActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Owner", DisplayName = "Create Action", BlueprintInternalUseOnly = "true"), Category = Action)
	static UAction* CreateAction(const TScriptInterface<IActionOwnerInterface>& Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate = false) {
		return UAction::Create(Owner, Type, bAutoActivate);
	}

};