// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxUI : ModuleRules
{
	public WxUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"CommonInput",
				"CommonUI",
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				"ModelViewViewModel",
				"UMG",
				"WxCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			}
		);

		if (Target.bBuildEditor)
		{
			// 디자인타임 EI 아이콘 프리뷰(WxActionWidget::GetIcon)에서만 사용한다.
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"AssetRegistry",
					"EnhancedInput",
				}
			);
		}
	}
}
