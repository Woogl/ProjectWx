// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/WxCombatAttributeSet.h"
#include "AbilitySystem/Effects/WxEffect_DrainGP.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

UWxAbility_Groggy::UWxAbility_Groggy()
{
	// 로컬 조종 액터에는 복제 몽타주가 적용되지 않아 소유 클라도 활성화해야 한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Groggy);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Groggy);
	
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	// 반응은 끊지 않아, 처형 짝 피격처럼 겹쳐야 할 반응이 보존된다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Groggy;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GetMontage() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartMontagePolling();

	// 클라의 복제 GP로 종료를 판정하면 서버와 종료 시점이 어긋난다.
	if (ActorInfo->IsNetAuthority())
	{
		StartGroggyDrain(Handle, ActorInfo, ActivationInfo);
	}

	HandleMontagePollTick();
}

void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	StopMontagePolling();

	if (ActorInfo)
	{
		if (ActorInfo->AbilitySystemComponent.IsValid())
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

			// 가산 피격이 GAS 추적 자리를 차지해도 관전자에게 그로기 정지를 전달한다.
			UAnimMontage* GroggyMontage = GetMontage();
			UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
			UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(ASC);
			if (GroggyMontage && WxASC && ActorInfo->IsNetAuthority())
			{
				WxASC->MulticastStopMontage(GroggyMontage);
			}
			else if (GroggyMontage && AnimInstance)
			{
				AnimInstance->Montage_Stop(GroggyMontage->GetDefaultBlendOutTime(), GroggyMontage);
			}

			StopGroggyDrain(*ASC);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Groggy::HandleGPChanged(const FOnAttributeChangeData& Data)
{
	if (FMath::IsNearlyZero(Data.NewValue) || Data.NewValue < 0.f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::HandleMontagePollTick()
{
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
	{
		// 사망은 액션만 끊어 그로기를 취소하지 않으므로 여기서 종료한다.
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// GAS는 마지막에 재생한 몽타주 하나만 기록해, 그 위에 겹친 가산 피격이 끝나면 아직 도는 처형 짝 몽타주를 놓친다 — 실제 재생 상태를 본다.
	if (const UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance(); AnimInstance && AnimInstance->Montage_IsActive(nullptr))
	{
		return;
	}

	PlayMontage(GetMontage());
}

bool UWxAbility_Groggy::PlayMontageInternal(UAnimMontage* Montage, FName StartSection)
{
	// 그로기는 몽타주 종료가 아니라 GP 드레인으로 끝나며, 중간 피격 뒤에는 폴링으로 자세를 복구한다.
	// 방향을 기다리는 사이 다른 몽타주가 시작됐으면 다음 폴링까지 기다린다.
	if (const UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr; AnimInstance && AnimInstance->Montage_IsActive(nullptr))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC && ASC->PlayMontage(this, CurrentActivationInfo, Montage, 1.f, StartSection) > 0.f;
}

void UWxAbility_Groggy::StartMontagePolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MontagePollingTimerHandle, this, &UWxAbility_Groggy::HandleMontagePollTick, 0.1f, true);
	}
}

void UWxAbility_Groggy::StopMontagePolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MontagePollingTimerHandle);
	}
}

void UWxAbility_Groggy::StartGroggyDrain(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	GPDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute())
		.AddUObject(this, &UWxAbility_Groggy::HandleGPChanged);

	const float GroggyDuration = GetMontage()->GetPlayLength();
	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainGP::StaticClass(), GetAbilityLevel());
	if (DrainSpecHandle.IsValid())
	{
		// 잠가서 넣어야 적용 시점의 Def 기반 Duration 재계산이 이 값을 덮어쓰지 않는다.
		DrainSpecHandle.Data->SetDuration(GroggyDuration, true);
		DrainGPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
	}
}

void UWxAbility_Groggy::StopGroggyDrain(UAbilitySystemComponent& ASC)
{
	if (DrainGPEffectHandle.IsValid())
	{
		ASC.RemoveActiveGameplayEffect(DrainGPEffectHandle);
		DrainGPEffectHandle.Invalidate();
	}

	if (GPDelegateHandle.IsValid())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute()).Remove(GPDelegateHandle);
		GPDelegateHandle.Reset();
	}
}
