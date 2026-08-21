// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class WxWorld : ModuleRules
{
	public WxWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			// 기믹 컴포넌트가 UStateTreeComponent(= UBrainComponent 파생) 를 상속하므로 공개 헤더가 두 모듈을 함께 노출한다.
			"AIModule",
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			// 레버의 공개 헤더가 태그 요건(FGameplayTagRequirements) 을 저작 필드로 노출한다.
			"GameplayAbilities",
			"GameplayStateTreeModule",
			"GameplayTags",
			"ModularGameplay",
			"StateTreeModule",
			"UniversalObjectLocator",
			"WxCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
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
