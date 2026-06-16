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
			"GameplayTags",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayAbilities",
			"LevelSequence",
			"MovieScene",
			"Niagara",
		});
	}
}
