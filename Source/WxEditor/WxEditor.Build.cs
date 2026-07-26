// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxEditor : ModuleRules
{
	public WxEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { ModuleDirectory });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayStateTreeModule",
			"SceneOutliner",
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"StateTreeEditorModule",
			"UniversalObjectLocator",
			"UniversalObjectLocatorEditor",
			"UnrealEd",
			"WxCombat",
			"WxCore",
			"WxInventory",
			"WxQuest",
			"WxUI",
		});
	}
}
