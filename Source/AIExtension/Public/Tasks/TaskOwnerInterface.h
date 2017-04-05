// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Interface.h"
#include "TaskOwnerInterface.generated.h"

class UTask;
class UTaskComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTaskOwnerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class AIEXTENSION_API ITaskOwnerInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    virtual void AddChildren(UTask* NewChildren) {}
    virtual void RemoveChildren(UTask* Children) {}

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Task)
    UTaskComponent* GetTaskOwnerComponent();

};
