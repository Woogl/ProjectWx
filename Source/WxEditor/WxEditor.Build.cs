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
			"GameplayAbilities",
			"ModelViewViewModel",
			"ModelViewViewModelBlueprint",
			"ModelViewViewModelEditor",
			"UMG",
			"UMGEditor",
			"GameplayTags",
			"GameplayStateTreeModule",
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"StateTreeEditorModule",
			"UniversalObjectLocator",
			"UnrealEd",
			"WxCombat",
			"WxCore",
			"WxInventory",
			"WxQuest",
			"WxUI",
			"WxWorld",
		});
	}
}
