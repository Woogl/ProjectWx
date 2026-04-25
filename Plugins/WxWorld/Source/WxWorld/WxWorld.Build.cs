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
			"Engine",
			"GameplayTags",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"GameplayAbilities",
			"LevelSequence",
			"MovieScene",
			"Niagara",
			"UMG",
		});
	}
}
