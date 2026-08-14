// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// 슬롯마다 다른 애셋 태그(Ability.Skill.1~4)와 입력 액션은 BP 서브클래스가 지정한다.
	// BP가 애셋 태그를 편집하면 컨테이너를 통째로 갖게 되므로, 여기 마커는 아직 편집하지 않은 신규 BP에만 상속된다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);

	// 슬롯 태그는 BP 소관이라 코드가 알 수 없으므로 부모 태그로 활성 표식을 보장한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Skill);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 스킬은 재생 중 다른 GA로 캔슬되지 않는다. (PC규격서 §5.2)
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	bRetriggerInstancedAbility = true;
}

bool UWxAbility_Skill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	// 자기 차단은 곧 EndAbility가 푸니 무시한다.
	if (Spec && Spec->IsActive())
	{
		if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::State_ComboWindow))
		{
			return false;
		}
		if (ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags))
		{
			return false;
		}
		return CheckCooldown(Handle, ActorInfo, OptionalRelevantTags) && CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWxAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!PlayMontage(MontageSelector.AdvanceMontage(GetAbilitySystemComponentFromActorInfo())))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 콤보 재발동은 이 종료 직후 다음 단을 재생하므로 몽타주를 끊지 않고 그대로 넘긴다.
	// 이 호출이 구 태스크의 후속 이벤트도 막아, 뒤따르는 종료가 Interrupted 핸들러를 깨워 진행 상태를 되돌리는 일이 없다.
	KeepMontagePlayingAfterEnd();

	// 캔슬 종료는 전부 여기서 되돌린다 — 위 호출 탓에 몽타주 핸들러가 돌지 않는다.
	// 콤보 재발동은 bWasCancelled=false라 진행 상태가 보존된다.
	if (bWasCancelled)
	{
		MontageSelector.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	// 캔슬이 아니라 EndAbility가 되돌려주지 않는다.
	MontageSelector.Reset();

	Super::HandleMontageCompleted();
}
