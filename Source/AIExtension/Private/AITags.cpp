// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AITags.h"

/// This is just for readability, reducing code cluttering.
#define MANAGER UGameplayTagsManager::Get()


FName UAITags::IEnum(UPackage* Package, FName Name)
{
    const auto &Object = FindObject<UEnum>(Package, *Name.ToString(), true);

    if (Object && Object->HasMetaData(TEXT("TagParent")))
    {
        return *(Object->GetMetaData(TEXT("TagParent")));
    }
    return TEXT("");
}

FName UAITags::IField(UPackage* Package, FName Enum, int32 Field)
{
    const auto &Object = FindObject<UEnum>(Package, *Enum.ToString(), true);

    if (Object && Object->HasMetaData(TEXT("TagParent")))
    {
        const FString EnumName = Object->GetMetaData(TEXT("TagParent"));
        const FString FieldName = Object->GetNameStringByIndex(Field);
        return FName(*(EnumName + FString(".") + FieldName));
    }
    return TEXT("");
}

void UAITags::RegisterEnumAsGameplayTags(UPackage* Package, FName Name)
{
    const auto& EN = FindObject<UEnum>(Package, *Name.ToString(), true);

    if (EN)
    {
        MANAGER.AddNativeGameplayTag(IEnum(Package, *Name.ToString()));
        for (int32 E = 0; E < EN->NumEnums() - 1; E++)
        {
            MANAGER.AddNativeGameplayTag(IField(Package, *Name.ToString(), E));
        }
    }
}

void UAITags::RegisterGameplayTags()
{
    RegisterEnumAsGameplayTags(ANY_PACKAGE, TEXT("ECombatState"));
}

template <typename T>
FGameplayTag UAITags::FromEnum(FName Enum, const T EnumeratorValue)
{
    // For the C++ enum.
    static_assert(TIsEnum<T>::Value, "Should only call this with enum types");
    return MANAGER.RequestGameplayTag(IField(ANY_PACKAGE, Enum, (int64)EnumeratorValue), false);
}

#undef MANAGER
