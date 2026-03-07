// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WxCombat : ModuleRules
{
	public WxCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
		});
	}
}
