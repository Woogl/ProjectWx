// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Passive.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

UWxAbility_Passive::UWxAbility_Passive()
{
	// 항상 서버에서 발행된 GameplayEvent로 트리거되고, 지급 결과는 어트리뷰트 복제로 클라이언트에 닿는다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Passive);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Passive);
}

void UWxAbility_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 발동마다 새로 채워지는 예측 키가 곧 그 발동의 식별자다 — 인스턴스를 재사용하는 콤보 재발동도 새 키를 받는다.
	// 공격 어빌리티가 실리지 않은 독립 적중(반사 투사체 등)은 키가 무효라 그 히트를 한 번으로 쳐서 지급한다.
	const UGameplayAbility* SourceAbility = TriggerEventData ? TriggerEventData->ContextHandle.GetAbilityInstance_NotReplicated() : nullptr;
	const FPredictionKey SourceActivationKey = SourceAbility ? SourceAbility->GetCurrentActivationInfo().GetActivationPredictionKey() : FPredictionKey();
	if (SourceActivationKey.IsValidKey() && SourceActivationKey == ChargedActivationKey)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 독립 적중이 끼어들어도 이미 지급한 근접 발동의 다단 적중을 다시 지급하지 않는다.
	if (SourceActivationKey.IsValidKey())
	{
		ChargedActivationKey = SourceActivationKey;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : TriggeredEffects)
	{
		if (EffectClass)
		{
			ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, EffectClass.GetDefaultObject(), GetAbilityLevel());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
