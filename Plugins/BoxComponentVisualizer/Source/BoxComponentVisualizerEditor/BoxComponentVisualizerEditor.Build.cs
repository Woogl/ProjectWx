// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class BoxComponentVisualizerEditor : ModuleRules
{
	public BoxComponentVisualizerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
		});
	}
}
