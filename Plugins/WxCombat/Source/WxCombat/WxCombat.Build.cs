// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxCombat : ModuleRules
{
	public WxCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"Core",
			"CoreUObject",
			"EnhancedInput",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ModularGameplay",
			"NavigationSystem",
			"NetCore",
			"TargetingSystem",
			"UMG",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"LevelSequence",
			"MotionWarping",
			"MovieScene",
			"Niagara",
		});
	}
}
