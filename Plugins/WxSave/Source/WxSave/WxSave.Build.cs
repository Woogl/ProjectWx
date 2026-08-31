// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxSave : ModuleRules
{
	public WxSave(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"MassCore",
			"MassEntity",
			"MassSpawner",
			"StateTreeModule",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"EngineSettings",
			"InstancedActors",
			"LevelStreamingPersistence",
			"MassActors",
			"MassSimulation",
		});
	}
}
