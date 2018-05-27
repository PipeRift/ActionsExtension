// Copyright 2015-2018 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class UtilityTree : ModuleRules
{
    public UtilityTree(ReadOnlyTargetRules Target): base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add("UtilityTree/Public");
        
        PrivateIncludePaths.Add("UtilityTree/Private");


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );
            
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RenderCore",
                "CoreUObject",
                "Engine",
                "RHI",
                "Slate",
                "SlateCore"
            }
        );
    
    
        if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
        {
            PrivateDependencyModuleNames.Add("GameplayDebugger");
            Definitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
        }
        else
        {
            Definitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
        }
    }
}
