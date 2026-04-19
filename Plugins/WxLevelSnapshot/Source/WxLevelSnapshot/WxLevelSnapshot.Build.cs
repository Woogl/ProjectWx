// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxLevelSnapshot : ModuleRules
{
	public WxLevelSnapshot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				"AssetRegistry",
				"Projects",
			}
		);
	}
}
