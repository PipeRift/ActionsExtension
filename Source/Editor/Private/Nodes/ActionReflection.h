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

    UPROPERTY()
    FName Name;

    UPROPERTY()
    FEdGraphPinType Type;

public:

    FBaseActionProperty() {}

    FName GetFName() const { return Name; }
    const FEdGraphPinType& GetType() const { return Type; }
    UProperty* GetProperty() const { return Property; }

    bool IsValid() const { return Property != nullptr; }

    bool operator==(const FBaseActionProperty& Other) const
    {
        return Name == Other.Name && Type == Other.Type;
    }
    bool operator!=(const FBaseActionProperty& Other) const { return !operator==(Other); }

    friend int32 GetTypeHash(const FBaseActionProperty& Item)
    {
        return GetTypeHash(Item.GetFName());
    }

protected:

    FBaseActionProperty(UProperty* Property)
        : Property(Property)
    {
        RefreshProperty();
    }

private:

    void RefreshProperty();
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
    TSet<FVariableActionProperty> Variables;

    UPROPERTY()
    TSet<FDelegateActionProperty> SimpleDelegates;

    UPROPERTY()
    TSet<FDelegateActionProperty> ComplexDelegates;


    bool operator==(const FActionProperties& Other) const
    {
        if (Variables.Num() != Other.Variables.Num() ||
            SimpleDelegates.Num() != Other.SimpleDelegates.Num() ||
            ComplexDelegates.Num() != Other.ComplexDelegates.Num())
        {
            return false;
        }

        return Equals<FVariableActionProperty>(Variables, Other.Variables)
            && Equals<FDelegateActionProperty>(SimpleDelegates, Other.SimpleDelegates)
            && Equals<FDelegateActionProperty>(ComplexDelegates, Other.ComplexDelegates);
    }
    bool operator!=(const FActionProperties& Other) const { return !operator==(Other); }

private:

    template<typename T>
    bool Equals(const TSet<T>& One, const TSet<T>& Other) const
    {
        for(const T& Value : One)
        {
            const T* const OtherValue = Other.Find(Value);
            if (!OtherValue || *OtherValue != Value)
            {
                // Element not found or not equal
                return false;
            }
        }
        return true;
    }
};


namespace ActionReflection
{
    bool GetVisibleProperties(UClass* Class, FActionProperties& OutProperties);
};
