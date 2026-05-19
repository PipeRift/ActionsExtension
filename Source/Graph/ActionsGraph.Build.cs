// Copyright 2015-2026 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class ActionsGraph : ModuleRules
{
	public ActionsGraph(ReadOnlyTargetRules TargetRules) : base(TargetRules)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"ActionsExtension",
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
			"Kismet",
			"KismetCompiler",
			"BlueprintGraph"
		});
	}
}
