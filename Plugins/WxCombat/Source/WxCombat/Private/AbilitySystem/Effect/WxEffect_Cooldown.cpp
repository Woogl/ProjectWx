// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxCombatModule.h"
#include "WxGameplayTags.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FCustomCalculationBasedFloat DurationCalc;
	DurationCalc.CalculationClassMagnitude = UWxMMC_CooldownDuration::StaticClass();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationCalc);

	// 쿨다운은 언제나 자기 자신에게 걸리므로 소스를 따지지 않는다 — 인스티게이터 ASC가 비면 스택이 갈라지는 AggregateBySource와 달리 항상 하나로 모인다.
	// 상한은 테이블의 MaxRecharges라 GE에는 두지 않는다(0 = 무제한).
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}

void UWxEffect_Cooldown::GrantCooldownTag(const FGameplayTag& CooldownTag)
{
	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(CooldownTag);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);
}

float UWxMMC_CooldownDuration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly).
	// 정적 데이터라 서버/클라 동일.
	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetEffectContext().GetAbility());
	const float CooldownTime = Ability ? Ability->GetCooldownTime() : 0.f;
	if (CooldownTime > 0.f)
	{
		return CooldownTime;
	}

	// 어빌리티를 거치지 않고 적용되면 수치를 알 수 없다. 0을 내면 엔진이 만료 타이머를 걸지 않아 쿨다운이 영구히 남으므로 즉시 만료시킨다.
	UE_LOG(LogWxCombat, Warning, TEXT("%s: 소스 어빌리티에서 쿨다운 수치를 읽지 못해 즉시 만료시킨다."), *GetNameSafe(Spec.Def));
	return UE_KINDA_SMALL_NUMBER;
}

UWxEffect_Cooldown_Dodge::UWxEffect_Cooldown_Dodge()
{
	GrantCooldownTag(WxGameplayTags::Cooldown_Dodge);
}

UWxEffect_Cooldown_Skill_1::UWxEffect_Cooldown_Skill_1()
{
	GrantCooldownTag(WxGameplayTags::Cooldown_Skill_1);
}

UWxEffect_Cooldown_Ultimate::UWxEffect_Cooldown_Ultimate()
{
	GrantCooldownTag(WxGameplayTags::Cooldown_Ultimate);
}
