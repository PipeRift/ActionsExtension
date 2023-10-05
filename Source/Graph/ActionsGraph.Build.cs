// Copyright 2015-2020 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class ActionsGraph : ModuleRules
{
	public ActionsGraph(ReadOnlyTargetRules TargetRules) : base(TargetRules)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Actions",
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
			"KismetCompiler",
			"BlueprintGraph"
		});
	}
}
