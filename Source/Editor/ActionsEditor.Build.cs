// Copyright 2015-2019 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class ActionsEditor : ModuleRules
{
	public ActionsEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"UnrealEd",
			"Actions"
		});
	}
}
