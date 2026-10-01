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
			"CommonInput",
			"CommonUI",
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"EnhancedInput",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"MetaHumanSDKRuntime",
			"ModelViewViewModel",
			"MotionWarping",
			"NetCore",
			"StateTreeModule",
			"TargetingSystem",
			"UMG",
			"UniversalObjectLocator",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayStateTreeModule",
			"HairStrandsCore",
			"LevelSequence",
			"MovieScene",
			"NavigationSystem",
			"Niagara",
			"Slate",
			"SlateCore",
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
			PrivateDependencyModuleNames.Add("PropertyBindingUtils");
		}
	}
}
