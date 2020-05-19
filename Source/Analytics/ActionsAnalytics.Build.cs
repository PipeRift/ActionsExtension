// Copyright 2015-2019 Piperift. All Rights Reserved.

using System.IO;
using System;
using UnrealBuildTool;

public class ActionsAnalytics : ModuleRules
{
	public ActionsAnalytics(ReadOnlyTargetRules TargetRules) : base(TargetRules)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Actions"
		});


		string PlatformStr = Target.Platform.ToString();
		string LibName = "EOSSDK-" + PlatformStr + "-Shipping";

        string SDKPath = Path.Combine(ModuleDirectory, "SDK");
		string LibPath = Path.Combine(SDKPath, "Lib");
        string BinPath = Path.Combine(SDKPath, "Bin");

        string DestinationBinaryPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../..", "Binaries", PlatformStr));

		bool bIsLibrarySupported = false;
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
		{
			string DLLName = LibName + ".dll";
			PublicAdditionalLibraries.Add(Path.Combine(LibPath, LibName + ".lib"));
			RuntimeDependencies.Add(DestinationBinaryPath, Path.Combine(BinPath, DLLName));

			bIsLibrarySupported = true;
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			LibName = "lib" + LibName + ".so";

			PublicAdditionalLibraries.Add(Path.Combine(BinPath, LibName));
			RuntimeDependencies.Add(DestinationBinaryPath, Path.Combine(BinPath, LibName));

			bIsLibrarySupported = true;
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			LibName = "lib" + LibName + ".dylib";

			PublicAdditionalLibraries.Add(Path.Combine(BinPath, LibName));
			RuntimeDependencies.Add(DestinationBinaryPath, Path.Combine(BinPath, LibName));

			bIsLibrarySupported = true;
		}

		if (bIsLibrarySupported)
		{
			PublicIncludePaths.Add(Path.Combine(SDKPath, "Include"));
		}

		PublicDefinitions.Add(string.Format("HAS_ACTIONS_ANALYTICS={0}", bIsLibrarySupported ? 1 : 0));
	}
}
