// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GameplayTagsModule.h"

#include "AITags.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UAITags : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    static FName IEnum(UPackage* Package, FName Name);
    static FName IField(UPackage* Package, FName Enum, int32 Field);
    static void RegisterEnumAsGameplayTags(UPackage* Package, FName Name);
    static void RegisterGameplayTags();


    template <typename T>
    static FGameplayTag FromEnum(FName Enum, const T EnumeratorValue);

    UFUNCTION(BlueprintPure, Category = "Enum", meta = (DisplayName = "To Gameplay Tag"))
    static FORCEINLINE FGameplayTag FromCombatState(ECombatState Value) {
        return FromEnum(TEXT("ECombatState"), Value);
    }
};
