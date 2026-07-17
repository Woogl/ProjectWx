// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxSave : ModuleRules
{
	public WxSave(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"ModularGameplay",
			"WxCore",
		});
	}
}
