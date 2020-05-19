// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "ActionsEOSPlatform.h"
#include "ActionsAnalytics.h"

#if HAS_ACTIONS_ANALYTICS

namespace EOSSettings
{
	const char* ProductName = "Actions Extension";

	const char* ProductId    = "7c6731c8c9e94d83978da310fad9af58";
	const char* SandboxId    = "p-t22uab9w5zr8ey3np786c9lhy8nh3j";
	const char* DeploymentId = "7670a1a83e40490ea4774998b201cf95";
	const char* ClientId     = "xyza7891eB8E19dWbJgMW9klk7KNQOrf";
	const char* ClientSecret = "eHgXlV9J++f98osPUVtr7rzVms1fl6gtvij3WmT6UHo";
};


FActionsEOSPlatform FActionsEOSPlatform::Instance {};


bool FActionsEOSPlatform::Create()
{
	if (PlatformHandle)
	{
		return false;
	}
	// Init EOS SDK
	EOS_InitializeOptions SDKOptions;
	SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	SDKOptions.AllocateMemoryFunction = nullptr;
	SDKOptions.ReallocateMemoryFunction = nullptr;
	SDKOptions.ReleaseMemoryFunction = nullptr;
	SDKOptions.ProductName = EOSSettings::ProductName;
	SDKOptions.ProductVersion = "0.1";
	SDKOptions.Reserved = nullptr;
	SDKOptions.SystemInitializeOptions = nullptr;

	EOS_EResult InitResult = EOS_Initialize(&SDKOptions);
	if (InitResult != EOS_EResult::EOS_Success)
	{
		UE_LOG(LogActionsAnalytics, Error, TEXT("[EOS SDK] Failed to init EOS."));
		return false;
	}

	// Create platform instance
	EOS_Platform_Options PlatformOptions = {};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.bIsServer = EOS_FALSE;
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;

	static std::string EncryptionKey(64, '1');
	PlatformOptions.EncryptionKey = EncryptionKey.c_str();
	PlatformOptions.Flags = 0;

	static const FString TempPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() + "/Temp/");
	if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*TempPath))
	{
		if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*TempPath))
		{
			UE_LOG(LogActionsAnalytics, Error, TEXT("[EOS SDK] Failed to create Cache Directory: %s."), *TempPath);
			return false;
		}
	}
	PlatformOptions.CacheDirectory = TCHAR_TO_ANSI(*TempPath);

	PlatformOptions.ProductId = EOSSettings::ProductId;
	PlatformOptions.SandboxId = EOSSettings::SandboxId;
	PlatformOptions.DeploymentId = EOSSettings::DeploymentId;

	if (std::strlen(PlatformOptions.ProductId) <= 0 ||
		std::strlen(PlatformOptions.SandboxId) <= 0 ||
		std::strlen(PlatformOptions.DeploymentId) <= 0)
	{
		UE_LOG(LogActionsAnalytics, Error, TEXT("[EOS SDK] Wrong product parameters!"));
		return false;
	}

	const bool bHasClientId     = std::strlen(EOSSettings::ClientId) > 0;
	const bool bHasClientSecret = std::strlen(EOSSettings::ClientSecret) > 0;
	if (bHasClientId && bHasClientSecret) // If one client parameter is missing, fail
	{
		PlatformOptions.ClientCredentials.ClientId = EOSSettings::ClientId;
		PlatformOptions.ClientCredentials.ClientSecret = EOSSettings::ClientSecret;
	}
	else if (bHasClientId != bHasClientSecret) // If one client parameter is missing, fail
	{
		UE_LOG(LogActionsAnalytics, Error, TEXT("[EOS SDK] Wrong client parameters!"));
		return false;
	}

	PlatformHandle = EOS_Platform_Create(&PlatformOptions);

	if (!PlatformHandle)
	{
		UE_LOG(LogActionsAnalytics, Error, TEXT("[EOS SDK] Failed to create Platform."));
		return false;
	}

	UE_LOG(LogActionsAnalytics, Verbose, TEXT("[EOS SDK] Initialized."));

	Metrics = MakeUnique<FActionsEOSMetrics>(*this);
	return true;
}

void FActionsEOSPlatform::Release()
{
	if (PlatformHandle)
	{
		Metrics.Reset();
		EOS_Platform_Release(PlatformHandle);
		EOS_Shutdown();
		PlatformHandle = nullptr;
	}
}

#endif
