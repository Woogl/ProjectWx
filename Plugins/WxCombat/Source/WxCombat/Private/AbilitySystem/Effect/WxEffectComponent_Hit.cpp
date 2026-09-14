// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_Hit.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Effect/WxEffect_AddAttribute.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Damage/WxDamageTableRow.h"
#include "Damage/WxHitEffectContext.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"

bool UWxEffectComponent_Hit::CanGameplayEffectApply(const FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec) const
{
	const UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
	const UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();
	const FWxHitEffectContext* Context = FWxHitEffectContext::Get(GESpec.GetContext());
	return Source && Target && Context && Context->DamageTable.IsValid()
		&& Context->DamageTable->FindRow<FWxDamageTableRow>(Context->DamageRowName, TEXT("HitRequirements"), false)
		&& UWxCombatLibrary::IsHostile(Source->GetAvatarActor(), Target->GetAvatarActor());
}

void UWxEffectComponent_Hit::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
	UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();
	FWxHitEffectContext* Context = FWxHitEffectContext::Get(GESpec.GetContext());
	if (!Source || !Target || !Context || !Context->DamageTable.IsValid())
	{
		return;
	}

	Context->ResetDamageResult();
	if (Target->HasMatchingGameplayTag(WxGameplayTags::Effect_Invincible))
	{
		FGameplayEventData EventData;
		EventData.EventTag = WxGameplayTags::Event_DodgeSuccess;
		EventData.Instigator = GESpec.GetContext().GetInstigator();
		EventData.Target = Target->GetOwnerActor();
		EventData.ContextHandle = GESpec.GetContext();
		Target->HandleGameplayEvent(WxGameplayTags::Event_DodgeSuccess, &EventData);
		return;
	}

	const FWxDamageTableRow* Row = Context->DamageTable->FindRow<FWxDamageTableRow>(Context->DamageRowName, TEXT("Hit"), false);
	if (!Row)
	{
		return;
	}
	const bool bCanGuard = GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanGuard);
	Context->bPerfectGuard = bCanGuard && Target->HasMatchingGameplayTag(WxGameplayTags::Effect_PerfectGuard);
	Context->bGuarded = bCanGuard && !Context->bPerfectGuard && Target->HasMatchingGameplayTag(WxGameplayTags::Effect_GuardReduction);

	FGameplayEffectSpec DamageSpec;
	DamageSpec.InitializeFromLinkedSpec(GetDefault<UWxEffect_Damage>(), GESpec);
	// 같은 타격의 Context를 공유하되 LinkedSpec의 Source 태그 스냅샷은 유지한다.
	DamageSpec.SetContext(GESpec.GetContext(), true);
	// LinkedSpec은 SetByCaller를 복사하지만 DynamicAssetTags 자체는 복사하지 않는다.
	DamageSpec.AppendDynamicAssetTags(GESpec.GetDynamicAssetTags());
	TArray<FGameplayEffectSpecHandle> ExtraSpecs;
	// 피해와 반응 이벤트가 소스 상태를 바꾸기 전에 추가 효과의 태그를 캡처한다.
	if (!Context->bPerfectGuard)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Row->AdditionalEffects)
		{
			if (EffectClass)
			{
				ExtraSpecs.Add(Source->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), GESpec.GetContext()));
			}
		}
	}
	const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(DamageSpec, Target, PredictionKey);
	Context->bDamageApplied = AppliedHandle.WasSuccessfullyApplied();
	if (!Context->bDamageApplied)
	{
		return;
	}

	// Cue나 반응 이벤트가 다른 효과를 적용하기 전에 이번 실행 결과를 보관한다.
	const float Damage = Context->DamageMagnitude;
	const float Reflect = Context->ReflectMagnitude;
	const bool bHasReflect = Context->bHasReflect;
	const bool bAuthority = Target->IsOwnerActorAuthoritative();
	DamageSpec.AppendDynamicAssetTags(Context->DamageResultTags);
	// 예측 클라는 Execution을 실행하지 않는다. 서버의 출력 없는 0 피해만 타격 Cue를 생략한다.
	// 반사량이 0이어도 퍼펙트 가드의 출력 기록은 남으므로 타격 연출을 유지한다.
	if (!bAuthority || Damage > 0.f || bHasReflect)
	{
		FGameplayCueParameters HitCue;
		UAbilitySystemGlobals::Get().InitGameplayCueParameters_GESpec(HitCue, DamageSpec);
		// 서버는 Damage 적용 시점, Execution을 건너뛰는 예측 클라는 Wrapper의 태그를 사용한다.
		HitCue.AggregatedTargetTags = bAuthority
			? Context->DamageTargetTags : *GESpec.CapturedTargetTags.GetAggregatedTags();
		UAbilitySystemGlobals::Get().GetGameplayCueManager()->InvokeGameplayCueExecuted_WithParams(Target, WxGameplayTags::GameplayCue_Hit, PredictionKey, HitCue);
	}

	if (bAuthority)
	{
		if (Damage > 0.f && DamageSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Attack))
		{
			ProcessDamageTaken(Target, DamageSpec, Damage);
		}
		if (bHasReflect)
		{
			ProcessPerfectGuard(Target, DamageSpec, Reflect);
		}
	}

	for (const FGameplayEffectSpecHandle& ExtraSpec : ExtraSpecs)
	{
		if (ExtraSpec.IsValid())
		{
			Source->ApplyGameplayEffectSpecToTarget(*ExtraSpec.Data.Get(), Target, PredictionKey);
		}
	}
}

void UWxEffectComponent_Hit::ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const
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

void UWxEffectComponent_Hit::ProcessPerfectGuard(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float ReflectAmount) const
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

	// 가드 어빌리티의 구독 수명과 무관하게 이 GE에서 성립한 퍼펙트 가드 결과를 처리한다.
	if (SourceASC)
	{
		// 이미 그로기면 GP를 더해 남은 드레인 시간보다 회복을 늦추지 않는다.
		// 방어자 컨텍스트를 사용해야 반사 GP에 의한 그로기의 원인이 방어자로 기록된다.
		if (!SourceASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
		{
			UWxEffect_AddGP::Apply(SourceASC, ReflectAmount, ASC->MakeEffectContext());
		}

		// 저작으로 가르는 것은 리액션뿐이다 — 막아낸 대가인 GP는 어느 공격이든 들어간다.
		if (Spec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanParry))
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
