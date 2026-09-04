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
			"MotionWarping",
			"TargetingSystem",
			"UMG",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"LevelSequence",
			"MovieScene",
			"Niagara",
		});
	}
}
