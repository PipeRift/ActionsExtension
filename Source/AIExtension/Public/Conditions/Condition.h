// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Condition.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class AIEXTENSION_API UCondition : public UDataAsset
{ 
    GENERATED_UCLASS_BODY()

public:
	virtual bool Execute() {
		return ReceiveExecute();
	}

    UFUNCTION(BlueprintNativeEvent, Category = Condition, meta=( DisplayName="Execute" ))
    bool ReceiveExecute();

#if WITH_EDITOR
    virtual bool CanEditChange(const UProperty* Property) const override;

    /**
    * Called when a property on this object has been modified externally
    *
    * @param PropertyThatChanged the property that was modified
    */
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR
};