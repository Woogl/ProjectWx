// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Exhaust.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Exhaust::UWxEffect_Exhaust()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_Exhausted);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_Exhausted);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);

	// 소모할 때마다 다시 걸리므로, 인스턴스가 쌓이지 않게 하나로 묶는다.
	// TODO: StackingType 직접 대입은 5.7에서 deprecated이나 SetStackingType이 WITH_EDITOR 전용이라 런타임에서 링크가 깨진다.
	// 런타임 setter가 생기면 교체할 것.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}

void UWxEffect_Exhaust::ApplyTo(UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC)
	{
		return;
	}

	const UGameplayEffect* CDO = StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
