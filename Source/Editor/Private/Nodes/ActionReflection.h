// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once
#include <CoreMinimal.h>

#include "ActionReflection.generated.h"


USTRUCT()
struct FBaseActionProperty
{
    GENERATED_BODY()

protected:

    UPROPERTY()
    UProperty* Property = nullptr;


public:

    FBaseActionProperty() {}

    bool IsValid() const { return Property != nullptr; }

    FName GetFName() const { return Property->GetFName(); }

    UProperty* GetProperty() const { return Property; }

protected:

    FBaseActionProperty(UProperty* Property)
        : Property(Property)
    {}
};


USTRUCT()
struct FDelegateActionProperty : public FBaseActionProperty
{
    GENERATED_BODY()

    FDelegateActionProperty(UMulticastDelegateProperty* Property = nullptr)
        : FBaseActionProperty(Property)
    {}

    UMulticastDelegateProperty* GetDelegate() const
    {
        return CastChecked<UMulticastDelegateProperty>(Property);
    }

    UFunction* GetFunction() const
    {
        return GetDelegate()->SignatureFunction;
    }

    bool HasParams() const
    {
        auto* Function = GetFunction();
        return Function? Function->NumParms > 0 : false;
    }
};


USTRUCT()
struct FVariableActionProperty : public FBaseActionProperty
{
    GENERATED_BODY()

    FVariableActionProperty(UProperty* Property = nullptr)
        : FBaseActionProperty(Property)
    {}
};


USTRUCT()
struct FActionProperties
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVariableActionProperty> Variables;

    UPROPERTY()
    TArray<FDelegateActionProperty> SimpleDelegates;

    UPROPERTY()
    TArray<FDelegateActionProperty> ComplexDelegates;
};


namespace ActionReflection
{
    TOptional<FActionProperties> GetVisibleProperties(UClass* Class);
};
