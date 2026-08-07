// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// 슬롯마다 다른 애셋 태그(Ability.Skill.1~4)와 입력 액션(ActivationInputAction = IA_Skill_1~4)은 BP 서브클래스가 지정한다.
	// BP가 애셋 태그를 편집하면 컨테이너 값을 통째로 갖게 되므로, 여기 마커는 아직 편집하지 않은 신규 BP에만 상속된다(기존 4개는 에셋에 직접 넣어 둔다).
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 스킬은 재생 중 다른 GA로 캔슬되지 않는다. (PC규격서 §5.6)
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	// 콤보는 재발동으로 다음 단계로 넘어간다. 콤보 윈도우 판정은 CanActivateAbility가 담당한다.
	bRetriggerInstancedAbility = true;
}

bool UWxAbility_Skill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	// 활성 중 재발동 = 콤보 진행. 이 어빌리티가 건 자기 차단(Ability)은 곧 EndAbility가 해제하므로 무시하고,
	// 콤보 윈도우 안에서만 허용하되 사망/비용/쿨다운은 그대로 판정한다.
	if (Spec && Spec->IsActive())
	{
		if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
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

	if (SkillMontages.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 재발동으로 이어온 콤보면 다음 인덱스, 아니면(신규 발동/터미널) 첫 인덱스부터.
	// CurrentIndex는 재발동 사이 보존되고(INDEX_NONE이면 IsValidIndex(0)로 0에서 시작), 콤보 자연 종료 시 몽타주 핸들러가 INDEX_NONE으로 되돌린다.
	CurrentIndex = SkillMontages.IsValidIndex(CurrentIndex + 1) ? CurrentIndex + 1 : 0;

	PlayCurrentMontage();
}

void UWxAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 콤보 재발동 시 엔진이 이 EndAbility를 먼저 호출한다.
	// 몽타주 태스크를 콜백 해제(EndTask) 후 정리해, 그 종료가 Interrupted/Cancelled 핸들러를 깨워 CurrentIndex를 되돌리는 것을 막는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	// 외부 캔슬(HitReact/Guard/Dodge 등이 CancelAbilitiesWithTag로 취소)에서는 위 EndTask가 몽타주 핸들러를 끊어
	// CurrentIndex를 되돌리는 Interrupted/Cancelled 핸들러가 실행되지 않는다. 여기서 직접 리셋해 다음 신규 스킬이
	// 콤보 중간이 아니라 첫 인덱스부터 시작하게 한다. 콤보 재발동은 bWasCancelled=false로 들어오므로 진행 중 인덱스는 보존된다.
	if (bWasCancelled)
	{
		CurrentIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Skill::PlayCurrentMontage()
{
	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UAnimMontage* Montage = SkillMontages.IsValidIndex(CurrentIndex) ? SkillMontages[CurrentIndex].Get() : nullptr;
	if (!Montage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Skill::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Skill::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Skill::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Skill::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	// 콤보 미입력으로 자연 종료 → 콤보 리셋(다음 발동은 첫 인덱스부터).
	CurrentIndex = INDEX_NONE;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Skill::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Skill::HandleMontageInterrupted()
{
	CurrentIndex = INDEX_NONE;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Skill::HandleMontageCancelled()
{
	CurrentIndex = INDEX_NONE;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
