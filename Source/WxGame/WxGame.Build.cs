// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxGame : ModuleRules
{
	public WxGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { ModuleDirectory });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"CommonUI",
			"Core",
			"CoreUObject",
			"Engine",
			"GameFeatures",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"MetaHumanSDKRuntime",
			"ModelViewViewModel",
			"ModularGameplay",
			"MotionWarping",
			"UMG",
			"WxAI",
			"WxCombat",
			"WxCore",
			"WxDialogue",
			"WxInventory",
			"WxQuest",
			"WxUI",
			"WxWorld",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EnhancedInput",
			"HairStrandsCore",
		});
	}
}
