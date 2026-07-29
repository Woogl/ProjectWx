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
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"StateTreeEditorModule",
			"UnrealEd",
			"WxCombat",
			"WxCore",
			"WxGame",
			"WxInventory",
			"WxQuest",
			"WxUI",
		});
	}
}
