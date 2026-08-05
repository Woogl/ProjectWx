// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxQuest : ModuleRules
{
	public WxQuest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"ModularGameplay",
			"StateTreeModule",
			"UniversalObjectLocator",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayStateTreeModule",
		});
	}
}
