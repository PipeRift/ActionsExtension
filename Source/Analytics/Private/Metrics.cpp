// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Metrics.h"

#if HAS_ACTIONS_ANALYTICS
#include <HAL/Platform.h>

#include "ActionsSettings.h"
#include "ActionsEOSPlatform.h"


FActionsEOSMetrics::FActionsEOSMetrics(FActionsEOSPlatform& Platform)
    : Platform(Platform)
{
    check(Platform.IsInitialized());
    MetricsHandle = EOS_Platform_GetMetricsInterface(Platform.GetPlatformHandle());
}

FActionsEOSMetrics::~FActionsEOSMetrics()
{
	if (MetricsHandle && bStartedSession)
	{
		EndSession();
	}
}

bool FActionsEOSMetrics::BeginSession()
{
	if(!MetricsHandle)
	{
		UE_LOG(LogActionsAnalytics, Warning, TEXT("[EOS SDK] No Metrics found."));
		return false;
	}

    if(bStartedSession)
	{
		UE_LOG(LogActionsAnalytics, Warning, TEXT("[EOS SDK] Session already begun."));
		return false;
	}

	// Begin Player Session Metrics
	EOS_Metrics_BeginPlayerSessionOptions MetricsOptions = {};
	MetricsOptions.ApiVersion = EOS_METRICS_BEGINPLAYERSESSION_API_LATEST;

    SessionUserId = GetDefault<UActionsEditorSettings>()->GetAnalyticsUserId();
    const FString UserIdName = SessionUserId.ToString();
	MetricsOptions.AccountIdType = EOS_EMetricsAccountIdType::EOS_MAIT_External;
	MetricsOptions.AccountId.External = TCHAR_TO_UTF8(*UserIdName);
	MetricsOptions.DisplayName = TCHAR_TO_UTF8(*UserIdName);

    MetricsOptions.ControllerType = EOS_EUserControllerType::EOS_UCT_MouseKeyboard;

	MetricsOptions.ServerIp = nullptr;
	MetricsOptions.GameSessionId = nullptr;

	const EOS_EResult Result = EOS_Metrics_BeginPlayerSession( MetricsHandle, &MetricsOptions );
	if( Result != EOS_EResult::EOS_Success )
	{
		UE_LOG(LogActionsAnalytics, Warning, TEXT("[EOS SDK] Failed to begin session."));
		return false;
	}

    bStartedSession = true;
	return true;
}

bool FActionsEOSMetrics::EndSession()
{
	if(!MetricsHandle)
	{
		UE_LOG(LogActionsAnalytics, Warning, TEXT("[EOS SDK] No Metrics found."));
		return false;
	}

    if(!bStartedSession)
	{
		UE_LOG(LogActionsAnalytics, Warning, TEXT("[EOS SDK] No session to end."));
		return false;
	}

	// End Player Session Metrics
	EOS_Metrics_EndPlayerSessionOptions MetricsOptions = {};
	MetricsOptions.ApiVersion = EOS_METRICS_ENDPLAYERSESSION_API_LATEST;

    const FString UserIdName = SessionUserId.ToString();
	MetricsOptions.AccountIdType = EOS_EMetricsAccountIdType::EOS_MAIT_External;
	MetricsOptions.AccountId.External = TCHAR_TO_UTF8(*UserIdName);

    bStartedSession = false;

	const EOS_EResult Result = EOS_Metrics_EndPlayerSession( MetricsHandle, &MetricsOptions );
	if(Result != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogActionsAnalytics, Warning, TEXT("[EOS SDK] Failed to end session."));
        return false;
	}
    return true;
}
#endif
