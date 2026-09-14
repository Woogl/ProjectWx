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
	if (Duration <= 0.f || !Source || !Target || !Source->IsOwnerActorAuthoritative())
	{
		return;
	}

	// 히트스톱도 서버 적중 판정을 따르며 클라이언트는 GE 복제로 시작한다.
	const UGameplayAbility* AnimatingAbility = Source->GetAnimatingAbility();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.SetAbility(AnimatingAbility);

	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(StaticClass(), 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, Duration);
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, FPredictionKey());
}
