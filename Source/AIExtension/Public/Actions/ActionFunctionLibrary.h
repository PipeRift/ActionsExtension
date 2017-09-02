// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Action.h"
#include "ActionFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UActionFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (DefaultToSelf = "Owner", DisplayName = "Create Action", BlueprintInternalUseOnly = "true"), Category = Action)
    static UAction* CreateAction(const TScriptInterface<IActionOwnerInterface>& Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate = false);

};
