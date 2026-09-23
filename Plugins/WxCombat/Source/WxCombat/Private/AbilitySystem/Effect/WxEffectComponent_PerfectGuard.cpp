// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_PerfectGuard.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_AddAttribute.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Weapon/WxProjectileBase.h"
#include "WxGameplayTags.h"

void UWxEffectComponent_PerfectGuard::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
	if (!ASC || !GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_PerfectGuarded))
	{
		return;
	}

	const FGameplayEffectModifiedAttribute* ReflectRecord = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingReflectAttribute());
	const float ReflectAmount = ReflectRecord ? ReflectRecord->TotalMagnitude : 0.f;
	const FGameplayEffectContextHandle ContextHandle = GESpec.GetContext();
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
		if (GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanParry))
		{
			FGameplayEventData ParryEventData;
			ParryEventData.EventTag = WxGameplayTags::Event_Hit_Parry;
			ParryEventData.Instigator = ASC->GetOwnerActor();
			ParryEventData.Target = SourceASC->GetOwnerActor();
			SourceASC->HandleGameplayEvent(WxGameplayTags::Event_Hit_Parry, &ParryEventData);
		}
	}

	// 막힌 투사체는 방어자의 것이 되어 쏜 쪽으로 돌아간다. 투사체는 Owner 변화로 되돌림을 알고 파괴하지 않는다.
	AWxProjectileBase* Projectile = Cast<AWxProjectileBase>(ContextHandle.GetEffectCauser());
	APawn* Parrier = Cast<APawn>(ASC->GetAvatarActor());
	if (Projectile && Parrier)
	{
		Projectile->Reflect(*Parrier);
	}

	// UWxAbilitySystemGlobals가 원래 공격 컨텍스트의 ImpactPoint를 Cue 위치로 채운다.
	ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_PerfectGuard, ContextHandle);
}
