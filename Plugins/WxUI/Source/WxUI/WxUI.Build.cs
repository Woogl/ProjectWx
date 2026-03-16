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
				"CommonInput",
				"ModelViewViewModel",
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
				"GameplayAbilities",
				"GameplayTags",
				"DeveloperSettings",
			}
		);
	}
}
