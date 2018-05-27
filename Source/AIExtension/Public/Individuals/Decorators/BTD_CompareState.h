// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "AIGeneric.h"
#include "BTD_CompareState.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECompareStateMode : uint8
{
    Equals         UMETA(DisplayName = "Is Equal to"),
    Greater        UMETA(DisplayName = "is Greater than"),
    Less           UMETA(DisplayName = "is Less than"),
    GreaterOrEqual UMETA(DisplayName = "is Greater or Equal to"),
    LessOrEqual    UMETA(DisplayName = "is Less or Equal to"),
    NotEqual       UMETA(DisplayName = "is Not Equal than")
};

UCLASS()
class AIEXTENSION_API UBTD_CompareState : public UBTDecorator
{
    GENERATED_BODY()
    
public:

    UBTD_CompareState();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node)
    ECompareStateMode Comparison;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Node, meta = (DisplayName = "Towards"))
    ECombatState State;


public:

    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
    virtual FString GetStaticDescription() const override;
};
