// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxInventory : ModuleRules
{
	public WxInventory(ReadOnlyTargetRules Target) : base(Target)
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
			"NetCore",
			"StateTreeModule",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Niagara",
		});
	}
}
