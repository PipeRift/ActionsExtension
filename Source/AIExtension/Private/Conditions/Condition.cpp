// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "Condition.h"

#define LOCTEXT_NAMESPACE "Condition"

UCondition::UCondition(const FObjectInitializer & ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool UCondition::ReceiveExecute_Implementation()
{
    return true;
}

#if WITH_EDITOR

bool UCondition::CanEditChange(const UProperty* InProperty) const
{
    return Super::CanEditChange(InProperty);
}

void UCondition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE