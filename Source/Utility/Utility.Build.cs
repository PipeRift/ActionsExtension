// Copyright 2015-2017 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class Utility : ModuleRules
{
	public Utility(ReadOnlyTargetRules Target): base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add("Utility/Private");

		PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "AIModule",
				"GameplayTasks"
            }
        );
	}
}
