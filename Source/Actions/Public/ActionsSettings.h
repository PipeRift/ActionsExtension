// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include "ActionsSettings.generated.h"


UCLASS(config = EditorPerProjectUserSettings)
class ACTIONS_API UActionsEditorSettings : public UObject
{
    GENERATED_BODY()

private:

#if WITH_EDITORONLY_DATA
    // Anonymous user Id to identify the session on actions analytics.
    UPROPERTY(Config)
    FGuid AnalyticsUserId = FGuid::NewGuid();


public:

    const FGuid& GetAnalyticsUserId() const { return AnalyticsUserId; }
#endif
       
    virtual void PostInitProperties() override
    {
        Super::PostInitProperties();

        // Force Guid to be saved
        SaveConfig();
    }
};
