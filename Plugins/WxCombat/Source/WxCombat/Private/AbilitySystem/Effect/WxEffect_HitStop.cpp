// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_HitStop.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_HitStop::UWxEffect_HitStop()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_HitStop);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);
}

void UWxEffect_HitStop::Apply(float Duration, UAbilitySystemComponent* Source, UAbilitySystemComponent* Target)
{
	if (Duration <= 0.f || !Source || !Target)
	{
		return;
	}

	// 노티파이는 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다. 투사체처럼 몽타주가 끝난 뒤 맞으면 키 없이 권위로만 걸린다.
	const UGameplayAbility* AnimatingAbility = Source->GetAnimatingAbility();
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.SetAbility(AnimatingAbility);

	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(StaticClass(), 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, Duration);
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, PredictionKey);
}
