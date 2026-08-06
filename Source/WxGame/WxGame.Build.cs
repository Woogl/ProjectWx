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
			"WxSave",
			"WxUI",
			"WxWorld",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EnhancedInput",
			"HairStrandsCore",
			"MetaHumanSDKRuntime",
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
