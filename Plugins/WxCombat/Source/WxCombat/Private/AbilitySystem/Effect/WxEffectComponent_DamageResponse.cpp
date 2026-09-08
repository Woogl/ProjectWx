// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_DamageResponse.h"
#include "AbilitySystem/Effect/WxEffect_AddAttribute.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Weapon/WxProjectileBase.h"
#include "WxGameplayTags.h"

void UWxEffectComponent_DamageResponse::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);

	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
	if (!ASC)
	{
		return;
	}

	// 메타 속성은 이미 초기화됐다. GE 실행 기록으로 적용량을 읽어 HP 잔량에 따른 피해량 손실을 피한다.
	const FGameplayEffectModifiedAttribute* Damage = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingDamageAttribute());
	if (Damage && Damage->TotalMagnitude > 0.f && GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Attack))
	{
		// 속성 반영과 사망·그로기 처리가 끝난 뒤 현재 가드 상태로 반응을 결정한다.
		ProcessDamageTaken(ASC, GESpec, Damage->TotalMagnitude);
	}

	// 반사량이 0이어도 퍼펙트 가드는 성립하므로 값이 아닌 실행 기록의 존재로 판정한다.
	const FGameplayEffectModifiedAttribute* Reflect = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingReflectAttribute());
	if (Reflect)
	{
		ProcessPerfectGuard(ASC, GESpec, Reflect->TotalMagnitude);
	}
}

void UWxEffectComponent_DamageResponse::ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const
{
	const FGameplayEffectContextHandle ContextHandle = Spec.GetContext();

	AActor* TargetActor = ASC->GetOwnerActor();
	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();

	const FGameplayTag ReactionTag = Spec.GetDynamicAssetTags().Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First();

	// GuardReact가 같은 피격 이벤트로 흡수 몽타주를 틀므로, 가드로 막히지 않는 히트는 이벤트보다 먼저 가드를 끊어야 한다.
	// 반응 라우팅은 전부 Ability.Guard로 판정한다 — 여기만 Effect.GuardReduction을 보면 둘이 어긋난 상태에서 취소를 건너뛴 채 흡수 연출이 나간다.
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Guard) && !Spec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanGuard))
	{
		const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
		ASC->CancelAbilities(&GuardAbilityTags);
	}

	// 브레이크 여부를 반응 종류에 실어 보내는 이유: 어빌리티 트리거는 RPC라 어트리뷰트 복제보다 먼저 도착해, 소유 클라가 SP를 다시 읽으면 차감 전 값을 본다.
	// 받아 줄 GuardReact가 Ability.Guard를 요구하므로, 같은 히트의 GP로 뜬 그로기가 가드를 먼저 끊었으면 일반 반응으로 보낸다.
	const bool bGuardBroken = Spec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_GuardBreak) && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Guard);
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

	// 플로터는 큐 노티파이가 대상 액터 위치에서 직접 띄우므로 Location을 채우지 않는다.
	// 판정 태그를 통째로 넘겨 크리 표시 같은 갈래는 큐 쪽이 고른다.
	FGameplayCueParameters CueParams;
	CueParams.EffectContext = ContextHandle;
	CueParams.RawMagnitude = Damage;
	CueParams.AggregatedSourceTags = Spec.GetDynamicAssetTags();

	ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_DamageFloater, CueParams);
}

void UWxEffectComponent_DamageResponse::ProcessPerfectGuard(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float ReflectAmount) const
{
	const FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();

	// 컨텍스트를 함께 실어야 가드 리액션이 피격 이벤트와 같은 방식으로 원인 액터를 집는다.
	FGameplayEventData EventData;
	EventData.EventTag = WxGameplayTags::Event_PerfectGuard;
	EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	EventData.Target = ASC->GetOwnerActor();
	EventData.EventMagnitude = ReflectAmount;
	EventData.ContextHandle = ContextHandle;
	ASC->HandleGameplayEvent(WxGameplayTags::Event_PerfectGuard, &EventData);

	// 투사체는 되돌아가는 것 자체가 보복이라 공격자에게 GP와 패리 리액션을 겹쳐 넣지 않는다.
	// 되돌아가는 판정은 투사체 쪽과 같아야 한다 — Damage.CanParry가 없으면 파괴만 되므로 보복이 없고, 막아낸 대가인 GP는 들어가야 한다.
	const AActor* EffectCauser = ContextHandle.GetEffectCauser();
	const bool bCanParry = Spec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanParry);
	const bool bReflectedProjectile = bCanParry && EffectCauser && EffectCauser->IsA<AWxProjectileBase>();

	// 가드 어빌리티의 구독 수명과 무관하게 이 GE에서 성립한 퍼펙트 가드 결과를 처리한다.
	if (SourceASC && !bReflectedProjectile)
	{
		// 이미 그로기면 GP를 더해 남은 드레인 시간보다 회복을 늦추지 않는다.
		// 방어자 컨텍스트를 사용해야 반사 GP에 의한 그로기의 원인이 방어자로 기록된다.
		if (!SourceASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
		{
			UWxEffect_AddGP::Apply(SourceASC, ReflectAmount, ASC->MakeEffectContext());
		}

		// 저작으로 가르는 것은 리액션뿐이다 — 막아낸 대가인 GP는 어느 공격이든 들어간다.
		if (bCanParry)
		{
			FGameplayEventData ParryEventData;
			ParryEventData.EventTag = WxGameplayTags::Event_Hit_Parry;
			ParryEventData.Instigator = ASC->GetOwnerActor();
			ParryEventData.Target = SourceASC->GetOwnerActor();
			SourceASC->HandleGameplayEvent(WxGameplayTags::Event_Hit_Parry, &ParryEventData);
		}
	}

	// UWxAbilitySystemGlobals가 원래 공격 컨텍스트의 ImpactPoint를 Cue 위치로 채운다.
	ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_PerfectGuard, ContextHandle);
}
