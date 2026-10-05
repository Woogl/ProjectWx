// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxToolset : ModuleRules
{
	public WxToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"BlueprintGraph",
			// LandscapeEdit.h 가 InstancedFoliageActor.h 를 포함한다.
			"Foliage",
			"GameplayStateTreeModule",
			"Json",
			"JsonUtilities",
			"Landscape",
			"ModelViewViewModel",
			"ModelViewViewModelBlueprint",
			"ModelViewViewModelEditor",
			"PropertyBindingUtils",
			"StateTreeEditorModule",
			"StateTreeModule",
			"ToolsetRegistry",
			"UMGEditor",
			"UnrealEd",
			"Water",
		});
	}
}
