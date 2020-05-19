// Copyright 2015-2020 Piperift. All Rights Reserved.
#pragma once

#include <CoreMinimal.h>
#include "ActionsAnalytics.h"

#if HAS_ACTIONS_ANALYTICS

#include "eos_sdk.h"
#include "Metrics.h"


/**
 * Creates EOS SDK Platform
 */
class ACTIONSANALYTICS_API FActionsEOSPlatform
{
	TUniquePtr<FActionsEOSMetrics> Metrics;

private:

	EOS_HPlatform PlatformHandle = nullptr;

	static FActionsEOSPlatform Instance;


public:
	FActionsEOSPlatform() {};
	FActionsEOSPlatform(FActionsEOSPlatform const&) = delete;
	FActionsEOSPlatform& operator=(FActionsEOSPlatform const&) = delete;
	virtual ~FActionsEOSPlatform() { Release(); };

	bool Create();

	void Release();

	bool IsInitialized() const { return PlatformHandle != nullptr; }
	EOS_HPlatform const GetPlatformHandle() const { return PlatformHandle; };

	FActionsEOSMetrics* GetMetrics() const { return Metrics.Get(); }

	static FActionsEOSPlatform& Get() { return Instance; }

};
#endif
