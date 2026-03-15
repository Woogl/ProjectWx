// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxUI : ModuleRules
{
	public WxUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"WxCore",
				"CommonUI",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"GameplayTags",
				"DeveloperSettings",
			}
		);
	}
}
