// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxAI : ModuleRules
{
	public WxAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"WxCore",
		});
	}
}
