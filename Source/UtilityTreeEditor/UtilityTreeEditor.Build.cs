// Copyright 2015-2017 Piperift. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class UtilityTreeEditor : ModuleRules
{
	public UtilityTreeEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        var EngineDir = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);

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
                "AIExtension",
                "UtilityTree"
            }
        );

        //Version specific dependencies
        BuildVersion Version;
        if (BuildVersion.TryRead(out Version))
        {
            //Is 4.18 or greater
            if (Version.MinorVersion >= 18)
            {
                PrivateDependencyModuleNames.Add("ApplicationCore");
            }
        }
	}
}
