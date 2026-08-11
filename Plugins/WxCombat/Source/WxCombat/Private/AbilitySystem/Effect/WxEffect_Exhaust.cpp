// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Exhaust.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Exhaust::UWxEffect_Exhaust()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ConsumeDelay));

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer TagChanges;
	TagChanges.Added.AddTag(WxGameplayTags::State_Exhausted);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagChanges);
	GEComponents.Add(TargetTagsComp);

	// 소모할 때마다 다시 걸리므로, 인스턴스가 쌓이지 않게 하나로 묶고 재적용을 지속시간 갱신으로 받는다.
	// TODO: StackingType 직접 대입은 5.7에서 deprecated이나 SetStackingType이 WITH_EDITOR 전용이라 런타임에서 링크가 깨진다.
	// 런타임 setter가 생기면 교체할 것.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}

void UWxEffect_Exhaust::ApplyTo(UAbilitySystemComponent* TargetASC, float Duration)
{
	if (!TargetASC || Duration <= 0.f)
	{
		return;
	}

	const UGameplayEffect* CDO = StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);

	// 잠그지 않으면 적용 단계에서 정의의 지속시간을 다시 계산해 덮어쓴다.
	Spec.SetDuration(Duration, true);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
