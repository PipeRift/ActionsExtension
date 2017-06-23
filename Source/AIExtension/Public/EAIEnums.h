// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EAIEnums.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
    EPassive        UMETA(DisplayName = "Passive"),
    ESuspicion      UMETA(DisplayName = "Suspicion"),
    EAlert          UMETA(DisplayName = "Alert"),
    ECombat         UMETA(DisplayName = "Combat")
};