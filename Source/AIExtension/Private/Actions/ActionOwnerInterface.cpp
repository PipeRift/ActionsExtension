// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"

#include "Action.h"
#include "ActionManagerComponent.h"

#include "ActionOwnerInterface.h"


// This function does not need to be modified.
UActionOwnerInterface::UActionOwnerInterface(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

const bool IActionOwnerInterface::AddChildren(UAction* NewChildren)
{
    if (UActionManagerComponent* comp = GetTaskOwnerComponent()) {
        return comp->AddChildren(NewChildren);
    }
    return false;
}

const bool IActionOwnerInterface::RemoveChildren(UAction* Children)
{
    if (UActionManagerComponent* comp = GetTaskOwnerComponent()) {
        return comp->RemoveChildren(Children);
    }
    return false;
}
