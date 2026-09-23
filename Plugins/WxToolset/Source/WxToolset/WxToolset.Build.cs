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
			"GameplayStateTreeModule",
			"Json",
			"JsonUtilities",
			"ModelViewViewModel",
			"ModelViewViewModelBlueprint",
			"ModelViewViewModelEditor",
			"PropertyBindingUtils",
			"StateTreeEditorModule",
			"StateTreeModule",
			"ToolsetRegistry",
			"UMGEditor",
			"UnrealEd",
		});
	}
}
