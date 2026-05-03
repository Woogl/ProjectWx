// Copyright Woogle. All Rights Reserved.

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
			"NetCore",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"LevelSequence",
			"MovieScene",
			"Niagara",
			"MotionWarping",
			"TargetingSystem",
			"UMG",
		});
	}
}
