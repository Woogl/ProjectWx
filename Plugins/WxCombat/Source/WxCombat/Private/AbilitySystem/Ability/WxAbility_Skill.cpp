// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_CostMP.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Skill);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	ActivationInputTag = WxGameplayTags::Input_Skill;
	CooldownTag = WxGameplayTags::Cooldown_Skill;
}

void UWxAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SkillMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Skill::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Skill::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Skill::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Skill::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

bool UWxAbility_Skill::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}
	
	if (MPCost <= 0.f)
	{
		return true;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	const UWxCombatAttributeSet* AttrSet = ASC->GetSet<UWxCombatAttributeSet>();
	if (!AttrSet || AttrSet->GetMP() < MPCost)
	{
		return false;
	}

	return true;
}

void UWxAbility_Skill::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (MPCost > 0.f)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_CostMP::StaticClass(), GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Cost, -MPCost);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	if (MaxCharges > 1)
	{
		CurrentCharges = FMath::Max(CurrentCharges - 1, 0);
	}
}

void UWxAbility_Skill::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (MaxCharges <= 1 || !CooldownTag.IsValid())
	{
		return;
	}

	CurrentCharges = MaxCharges;

	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		CooldownTagDelegateHandle = ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWxAbility_Skill::HandleCooldownTagChanged);
	}
}

void UWxAbility_Skill::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (MaxCharges > 1 && CooldownTagDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
				.Remove(CooldownTagDelegateHandle);
			CooldownTagDelegateHandle.Reset();
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UWxAbility_Skill::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (MaxCharges <= 1)
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	return CurrentCharges > 0;
}

void UWxAbility_Skill::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (MaxCharges <= 1)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	// 충전 모드: 쿨다운(충전 타이머)이 이미 진행 중이면 중복 적용하지 않는다
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC && !ASC->HasMatchingGameplayTag(CooldownTag))
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
	}
}

void UWxAbility_Skill::HandleCooldownTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount != 0)
	{
		return;
	}

	// 충전 타이머 만료: 충전 1 증가
	CurrentCharges = FMath::Min(CurrentCharges + 1, MaxCharges);

	// 아직 최대 충전에 도달하지 않았으면 다음 충전을 위해 타이머 재시작
	if (CurrentCharges < MaxCharges)
	{
		RestartChargeCooldown();
	}
}

void UWxAbility_Skill::RestartChargeCooldown()
{
	if (!CurrentActorInfo || CooldownDuration <= 0.f || !CooldownTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	const UGameplayEffect* CooldownGE = UWxEffect_Cooldown::StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CooldownGE, ASC->MakeEffectContext(), GetAbilityLevel());
	Spec.SetDuration(CooldownDuration, true);
	Spec.DynamicGrantedTags.AddTag(CooldownTag);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Skill::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Skill::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Skill::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
