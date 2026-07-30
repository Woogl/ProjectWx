// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxDialogue : ModuleRules
{
	public WxDialogue(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"ModularGameplay",
			"StateTreeModule",
			"WxCore",
		});
	}
}
