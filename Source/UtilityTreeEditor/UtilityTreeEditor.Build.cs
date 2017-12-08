// Copyright 2015-2017 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class UtilityTreeEditor : ModuleRules
{
	public UtilityTreeEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add("UtilityTreeEditor/Public");

        PrivateIncludePaths.Add("UtilityTreeEditor/Private");
			
		
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
