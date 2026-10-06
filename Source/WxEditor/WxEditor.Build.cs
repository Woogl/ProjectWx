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
			"CommonInput",
			"DeveloperSettings",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayStateTreeModule",
			"Json",
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"StateTreeEditorModule",
			"UniversalObjectLocator",
			"UnrealEd",
			"WxGame",
		});
	}
}
