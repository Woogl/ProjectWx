// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_DamageReaction.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

void UWxEffectComponent_DamageReaction::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
	if (!ASC)
	{
		return;
	}

	// 메타 속성은 이미 초기화됐다. 실행 기록을 사용해 남은 HP로 수치가 잘리지 않게 한다.
	const FGameplayEffectModifiedAttribute* DamageRecord = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingDamageAttribute());
	const float Damage = DamageRecord ? DamageRecord->TotalMagnitude : 0.f;
	if (Damage > 0.f)
	{
		FGameplayCueParameters FloaterCue;
		FloaterCue.EffectContext = GESpec.GetContext();
		FloaterCue.RawMagnitude = Damage;
		FloaterCue.AggregatedSourceTags = GESpec.GetDynamicAssetTags();
		ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_DamageFloater, FloaterCue);
	}

	// 반사량이 0이어도 퍼펙트 가드는 타격 연출을 유지한다.
	if (Damage > 0.f || GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_PerfectGuarded))
	{
		FGameplayCueParameters HitCue;
		UAbilitySystemGlobals::Get().InitGameplayCueParameters_GESpec(HitCue, GESpec);
		UAbilitySystemGlobals::Get().GetGameplayCueManager()->InvokeGameplayCueExecuted_WithParams(ASC, WxGameplayTags::GameplayCue_Hit, PredictionKey, HitCue);
	}

	if (Damage > 0.f)
	{
		ProcessDamageTaken(ASC, GESpec, Damage);
	}
}

void UWxEffectComponent_DamageReaction::ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const
{
	const FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
	const FGameplayTagContainer& DamageTags = Spec.GetDynamicAssetTags();

	AActor* TargetActor = ASC->GetOwnerActor();
	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();

	const FGameplayTag ReactionTag = DamageTags.Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First();

	// GuardReact가 같은 피격 이벤트로 흡수 몽타주를 틀므로, 가드로 막히지 않는 히트는 이벤트보다 먼저 가드를 끊어야 한다.
	// 반응 라우팅은 전부 Ability.Guard로 판정한다 — 여기만 Effect.GuardReduction을 보면 둘이 어긋난 상태에서 취소를 건너뛴 채 흡수 연출이 나간다.
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Guard) && !DamageTags.HasTag(WxGameplayTags::Damage_CanGuard))
	{
		const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
		ASC->CancelAbilities(&GuardAbilityTags);
	}

	// 브레이크 여부를 반응 종류에 실어 보내는 이유: 어빌리티 트리거는 RPC라 어트리뷰트 복제보다 먼저 도착해, 소유 클라가 SP를 다시 읽으면 차감 전 값을 본다.
	// 받아 줄 GuardReact가 Ability.Guard를 요구하므로, 같은 히트의 GP로 뜬 그로기가 가드를 먼저 끊었으면 일반 반응으로 보낸다.
	const bool bGuardBroken = DamageTags.HasTag(WxGameplayTags::Damage_GuardBreak) && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Guard);
	const FGameplayTag HitEventTag = bGuardBroken
		? WxGameplayTags::Event_Hit_GuardBreak
		: WxGameplayTags::Event_Hit;
	FGameplayEventData HitEventData;
	HitEventData.EventTag = HitEventTag;
	if (ReactionTag.IsValid())
	{
		HitEventData.TargetTags.AddTag(ReactionTag);
	}
	HitEventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	HitEventData.Target = TargetActor;
	HitEventData.EventMagnitude = Damage;
	HitEventData.ContextHandle = ContextHandle;
	ASC->HandleGameplayEvent(HitEventTag, &HitEventData);

	if (SourceASC)
	{
		FGameplayEventData DamageDealtEventData;
		DamageDealtEventData.EventTag = WxGameplayTags::Event_DamageDealt;
		DamageDealtEventData.Instigator = SourceASC->GetOwnerActor();
		DamageDealtEventData.Target = TargetActor;
		DamageDealtEventData.EventMagnitude = Damage;
		DamageDealtEventData.ContextHandle = ContextHandle;
		SourceASC->HandleGameplayEvent(WxGameplayTags::Event_DamageDealt, &DamageDealtEventData);
	}
}
