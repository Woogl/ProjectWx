// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_PerfectGuard.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_PerfectGuard::UWxEffect_PerfectGuard()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_PerfectGuard);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_PerfectGuard);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);
}

FActiveGameplayEffectHandle UWxEffect_PerfectGuard::ApplyTo(UAbilitySystemComponent* TargetASC, float Duration, const UGameplayAbility* PredictingAbility)
{
	if (!TargetASC || Duration <= 0.f)
	{
		return FActiveGameplayEffectHandle();
	}

	const UGameplayEffect* CDO = StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);

	// 잠그지 않으면 적용 단계에서 정의의 지속시간을 다시 계산해 덮어쓴다.
	Spec.SetDuration(Duration, true);

	// 애님 노티파이는 어빌리티 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (PredictingAbility)
	{
		PredictionKey = PredictingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	return TargetASC->ApplyGameplayEffectSpecToSelf(Spec, PredictionKey);
}
