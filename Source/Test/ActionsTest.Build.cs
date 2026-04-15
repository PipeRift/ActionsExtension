// Copyright 2015-2026 Piperift. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ActionsTest : ModuleRules
	{
		public ActionsTest(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

			PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core"
			});

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"ActionsExtension",
				"CoreUObject",
				"Engine",
				"EngineSettings"
			});

			if (Target.bBuildEditor)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}
		}
	}
}