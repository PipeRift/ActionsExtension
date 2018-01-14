// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "ActionFunctionLibrary.h"

UAction* UActionFunctionLibrary::CreateAction(const TScriptInterface<IActionOwnerInterface>& Owner, const TSubclassOf<class UAction> Type, bool bAutoActivate/* = false*/)
{
    if (!Owner.GetObject())
        return nullptr;

    if (!Type.Get() || Type == UAction::StaticClass())
        return nullptr;

    if (!bAutoActivate)
    {
        return NewObject<UAction>(Owner.GetObject(), Type);
    }
    else
    {
        UAction* Action = NewObject<UAction>(Owner.GetObject(), Type);
        Action->Activate();
        return Action;
    }
}
