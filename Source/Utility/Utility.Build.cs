// Fill out your copyright notice in the Description page of Project Settings.

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
