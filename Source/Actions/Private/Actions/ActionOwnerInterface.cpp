// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "ActionOwnerInterface.h"

#include "Action.h"
#include "ActionManagerComponent.h"


UActionOwnerInterface::UActionOwnerInterface(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

const bool IActionOwnerInterface::AddChildren(UAction* NewChildren)
{
	if (UActionManagerComponent* comp = GetActionOwnerComponent()) {
		return comp->AddChildren(NewChildren);
	}
	return false;
}

const bool IActionOwnerInterface::RemoveChildren(UAction* Children)
{
	if (UActionManagerComponent* comp = GetActionOwnerComponent()) {
		return comp->RemoveChildren(Children);
	}
	return false;
}
