// Copyright 2015-2020 Piperift. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ActionsTest : ModuleRules
	{
		public ActionsTest(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			IWYUSupport = IWYUSupport.Full;

			PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"Engine",
				"CoreUObject",
				"Actions"
			});

			PrivateDependencyModuleNames.AddRange(new string[]
			{
			});
		}
	}
}