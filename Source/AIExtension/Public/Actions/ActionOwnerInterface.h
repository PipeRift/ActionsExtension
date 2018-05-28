// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "ActionOwnerInterface.generated.h"

class UAction;
class UActionManagerComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UActionOwnerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class AIEXTENSION_API IActionOwnerInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual const bool AddChildren(UAction* NewChildren);

	virtual const bool RemoveChildren(UAction* Children);

	virtual UActionManagerComponent* GetActionOwnerComponent() { return nullptr; }
};
