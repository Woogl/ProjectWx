// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Sprint.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_DrainSP.h"
#include "AbilitySystem/Effect/WxEffect_MoveSpeedScale.h"
#include "AbilitySystem/Task/WxAbilityTask_WaitMoving.h"

UWxAbility_Sprint::UWxAbility_Sprint()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Sprint);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

bool UWxAbility_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UWxCombatAttributeSet* AttributeSet = ASC ? ASC->GetSet<UWxCombatAttributeSet>() : nullptr;

	// 진입 비용은 순정 CheckCost가 보므로, 여기서는 코스트가 0이어도 성립해야 하는 조건만 본다.
	// 고갈 상태에서 발동하면 소모 없이 달리면서 회복까지 막는 상태가 된다.
	// MaxSP가 없는 아바타는 스태미나를 쓰지 않는다 — 소모량도 0이라 제한 없이 달린다.
	if (AttributeSet && AttributeSet->GetMaxSP() > 0.f && AttributeSet->GetSP() <= 0.f)
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWxAbility_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	// 몽타주를 쓰지 않아 이동 컴포넌트의 앉기 취소에 걸리지 않으므로 여기서 직접 일으켜 세운다.
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->UnCrouch();
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_MoveSpeedScale::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, SprintSpeedScale);
		SpeedEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}

	// 소모 GE는 State.Sprint가 없으면 억제된다. 활성화 시 한 번 걸어 두고 이동 여부로 태그만 여닫는다.
	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainSP::StaticClass(), GetAbilityLevel());
	if (DrainSpecHandle.IsValid())
	{
		DrainEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
	}

	if (UWxAbilityTask_WaitMoving* MovingTask = UWxAbilityTask_WaitMoving::CreateTask(this))
	{
		MovingTask->OnMovingChanged.AddDynamic(this, &UWxAbility_Sprint::HandleMovingChanged);
		MovingTask->ReadyForActivation();
	}

	SPChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetSPAttribute())
		.AddUObject(this, &UWxAbility_Sprint::HandleSPChanged);
}

void UWxAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetSPAttribute()).Remove(SPChangedHandle);
		SPChangedHandle.Reset();

		// 정지 상태로 끝났으면 이미 빠져 있다.
		if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Sprint))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Sprint);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (ASC)
	{
		if (SpeedEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(SpeedEffectHandle);
			SpeedEffectHandle.Invalidate();
		}

		if (DrainEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(DrainEffectHandle);
			DrainEffectHandle.Invalidate();
		}
	}
}

void UWxAbility_Sprint::HandleMovingChanged(bool bIsMoving)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	if (bIsMoving)
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Sprint);
	}
	else if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Sprint))
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Sprint);
	}
}

void UWxAbility_Sprint::HandleSPChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue > 0.f)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
