// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxWorld : ModuleRules
{
	public WxWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"GameplayAbilities",
			"GameplayTags",
			"ModularGameplay",
			"StateTreeModule",
			"UniversalObjectLocator",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"GameplayStateTreeModule",
			"GameplayTasks",
			"LevelSequence",
			"MovieScene",
			"Niagara",
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
