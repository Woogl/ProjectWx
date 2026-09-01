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
				"CommonInput",
				"CommonUI",
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				"ModelViewViewModel",
				"ModularGameplay",
				"StateTreeModule",
				"UMG",
				"UniversalObjectLocator",
				"WxCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			}
		);
	}
}
