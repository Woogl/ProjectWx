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
			"StateTreeModule",
			"UniversalObjectLocator",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayAbilities",
			"GameplayStateTreeModule",
			"LevelSequence",
			"MovieScene",
			"Niagara",
		});

		if (Target.bBuildEditor)
		{
			// 스포너 라벨을 엔진 순정 규칙(FActorLabelUtilities::SetActorLabelUnique)으로 짓는 데만 사용한다.
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
