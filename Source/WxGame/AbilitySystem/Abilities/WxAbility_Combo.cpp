// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Combo.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"

UAnimMontage* UWxAbility_Combo::GetMontage() const
{
	const int32 Index = ComboIndex == INDEX_NONE ? 0 : ComboIndex;
	return ComboMontages.IsValidIndex(Index) ? ComboMontages[Index].Get() : nullptr;
}

float UWxAbility_Combo::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

void UWxAbility_Combo::OpenComboWindow()
{
	if (!IsActive() || !ActivationInputAction || InputTask)
	{
		return;
	}

	// 서버에서는 창보다 먼저 도착한 클라 입력을 엔진이 보관했다가 이 활성화 안에서 바로 넘긴다.
	InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	InputTask->OnPress.AddDynamic(this, &ThisClass::HandleComboInput);
	InputTask->ReadyForActivation();
}

void UWxAbility_Combo::CloseComboWindow()
{
	if (InputTask)
	{
		InputTask->EndTask();
		InputTask = nullptr;
	}
}

int32 UWxAbility_Combo::GetComboIndex() const
{
	return ComboIndex;
}

bool UWxAbility_Combo::TryMirrorComboStep(int32 Index)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->IsOwnerActorAuthoritative() || !ComboMontages.IsValidIndex(Index))
	{
		return false;
	}
	if (IsActive())
	{
		return PlayComboStep(Index);
	}
	TGuardValue<int32> StartGuard(StartingComboIndex, Index);
	return ASC->TryActivateAbility(CurrentSpecHandle, false);
}

void UWxAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	PlayComboStep(StartingComboIndex);
}

void UWxAbility_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ComboIndex = INDEX_NONE;
	InputTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWxAbility_Combo::PlayComboStep(int32 Index)
{
	CloseComboWindow();
	ComboIndex = Index;
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return false;
	}
	if (!IsActive())
	{
		return false;
	}
	ResetActionState();
	if (!PlayMontage(GetMontage()))
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return false;
	}
	return IsActive();
}

void UWxAbility_Combo::HandleComboInput(float TimeWaited)
{
	// 태스크는 이 브로드캐스트 직후 스스로 끝난다.
	InputTask = nullptr;
	PlayComboStep(ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0);
}
