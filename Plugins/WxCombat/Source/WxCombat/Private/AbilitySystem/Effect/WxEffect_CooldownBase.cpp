// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_CooldownBase.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UWxEffect_CooldownBase::UWxEffect_CooldownBase()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));

	// UE 5.3+ 부터 GameplayAbility는 쿨다운 GE가 UTargetTagsGameplayEffectComponent 를 통해
	// 태그를 Grant 해야 유효하다고 판정한다. 베이스에서 컴포넌트만 미리 부착해두고,
	// 실제 Cooldown.* 태그는 BP 차일드의 Components > Target Tags > Added 에서 설정한다.
	//
	// GEComponents는 Instanced UPROPERTY 배열이므로, CDO 시점에 들어가는 서브오브젝트는
	// 반드시 CreateDefaultSubobject 로 만들어야 한다. NewObject(=FindOrAddComponent) 로 넣으면
	// 패키지 시리얼라이즈/BP 컴파일 단계에서 에러가 난다.
	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComponent);

	// 충전 시스템: 사용 시 스택 +1, Duration 경과마다 스택 -1씩 순차 복구.
	// StackingType은 UE 5.7에서 deprecated 표시되었지만 SetStackingType은 WITH_EDITOR 한정 +
	// 언익스포트라 C++ 초기화 경로가 사실상 이것뿐이다. 엔진 자체도 같은 패턴으로 우회함.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1; // 서브클래스(BP)에서 MaxCharges로 override
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}
