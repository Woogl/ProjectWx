// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxCore : ModuleRules
{
	public WxCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UniversalObjectLocator",
			});
		}
	}
}
