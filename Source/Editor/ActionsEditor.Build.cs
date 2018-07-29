// Copyright 2015-2018 Piperift. All Rights Reserved.

using UnrealBuildTool;

public class ActionsEditor : ModuleRules
{
	public ActionsEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add( "Editor/Public" );

		PrivateIncludePaths.Add( "Editor/Private" );

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange( new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Blutility",
			"Slate",
			"SlateCore",
			"AssetTools",
			"EditorStyle",
			"KismetWidgets",
			"KismetCompiler",
			"BlueprintGraph",
			"GraphEditor",
			"Kismet",
			"PropertyEditor",
			"DetailCustomizations",
			"ContentBrowser",
			"Settings",
			"AIModule",
			"Actions"
		});
	}
}
