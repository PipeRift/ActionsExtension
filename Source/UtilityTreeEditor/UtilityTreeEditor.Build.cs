// Copyright 2015-2018 Piperift. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class UtilityTreeEditor : ModuleRules
{
	public UtilityTreeEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        var EngineDir = Path.GetFullPath(TargetRules.RelativeEnginePath);

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add("UtilityTreeEditor/Public");

        PrivateIncludePaths.Add("UtilityTreeEditor/Private");
		PrivateIncludePaths.AddRange(
            new string[] {
                "UtilityTreeEditor/Private",
                Path.Combine(EngineDir, @"Source/Developer/AssetTools/Private")
            });
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "InputCore",
                "Projects",
            }
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
            {
                "Core",
                "CoreUObject",
                "ApplicationCore",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "AssetTools",
                "EditorStyle",
                "ContentBrowser",
                "Kismet",
                "KismetCompiler",
                "BlueprintGraph",
                "GraphEditor",
                "AIExtension",
                "UtilityTree"
            }
        );
	}
}
