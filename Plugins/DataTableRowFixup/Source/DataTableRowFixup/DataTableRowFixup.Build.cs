// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class DataTableRowFixup : ModuleRules
{
	public DataTableRowFixup(ReadOnlyTargetRules Target) : base(Target)
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
			"AssetRegistry",
			"DeveloperSettings",
			"PropertyBindingUtils",
			"Slate",
			"SlateCore",
			"StateTreeEditorModule",
			"StateTreeModule",
			"UnrealEd",
		});
	}
}
