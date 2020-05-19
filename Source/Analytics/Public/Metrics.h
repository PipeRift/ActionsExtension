// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include "ActionsAnalytics.h"

#if HAS_ACTIONS_ANALYTICS
#include "eos_metrics.h"

class FActionsEOSPlatform;


struct ACTIONSANALYTICS_API FActionsEOSMetrics
{
private:

    bool bStartedSession = false;
	EOS_HMetrics MetricsHandle = nullptr;
    FGuid SessionUserId;


public:

    FActionsEOSPlatform& Platform;

    FActionsEOSMetrics(FActionsEOSPlatform& Platform);
    ~FActionsEOSMetrics();

    bool BeginSession();
    bool EndSession();
};

#endif
