// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxCombatModule.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWxAbilitySystemComponent::GiveAbilitySet()
{
	if (!AbilitySet)
	{
		return;
	}

	AbilitySet->GiveToAbilitySystem(this, &AbilitySetGrantedHandles);
}

void UWxAbilitySystemComponent::AbilityInputActionTriggered(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	// 순회 중 활성화가 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, RemoveAfterActivation 등).
	// 락이 없으면 Give/Clear가 즉시 Add/RemoveAtSwap 해 참조와 이터레이터가 무효화된다.
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability)
		{
			continue;
		}

		if (Ability->ActivationInputAction.Get() != Action)
		{
			continue;
		}

		// 신규 발동과 콤보 재발동은 엔진이 bRetriggerInstancedAbility로 가르므로 호출이 같다.
		if (TryActivateAbility(Spec.Handle))
		{
			break;
		}

		if (Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);

			for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

void UWxAbilitySystemComponent::AbilityInputActionReleased(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	// AbilityInputActionTriggered와 같은 이유로 락을 건다.
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability)
		{
			continue;
		}

		if (Ability->ActivationInputAction.Get() != Action)
		{
			continue;
		}

		if (Spec.IsActive())
		{
			AbilitySpecInputReleased(Spec);

			for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

TArray<const UInputAction*> UWxAbilitySystemComponent::GetAbilityInputActions() const
{
	if (AbilitySet)
	{
		return AbilitySet->GetInputActions();
	}
	return TArray<const UInputAction*>();
}

float UWxAbilitySystemComponent::GetMontagePlayRate() const
{
	const UWxCombatAttributeSet* AttrSet = GetSet<UWxCombatAttributeSet>();
	if (!AttrSet)
	{
		return 1.f;
	}

	return FMath::Max(AttrSet->GetASPD(), 0.01f);
}

void UWxAbilitySystemComponent::ApplyHitStop(float Duration, const UGameplayAbility* SourceAbility)
{
	// SetTimer가 0 이하를 예약 취소로 취급하므로, 그대로 두면 복원 없는 정지가 된다.
	if (Duration <= 0.f)
	{
		return;
	}

	if (!GetAnimatingAbility() || GetAnimatingAbility() != SourceAbility)
	{
		return;
	}

	UAnimMontage* Montage = GetCurrentMontage();
	if (!Montage)
	{
		return;
	}

	// 완전한 0이 아닌 미세 값으로 둬 몽타주 진행 판정 이슈를 피한다.
	CurrentMontageSetPlayRate(0.001f);

	// 연속 적중이면 타이머가 재설정되어 조기 복원을 막는다.
	GetWorld()->GetTimerManager().SetTimer(HitStopResumeTimer,
		FTimerDelegate::CreateUObject(this, &UWxAbilitySystemComponent::HandleHitStopElapsed, TWeakObjectPtr<UAnimMontage>(Montage)),
		Duration, false);
}

void UWxAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	UE_LOG(LogWxCombat, Verbose, TEXT("Ability Failed: %s — 사유 %s"), *GetNameSafe(Ability), *FailureReason.ToStringSimple());
}

const UWxAbilityBase* UWxAbilitySystemComponent::FindActivationGroupBlocker() const
{
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		for (const UGameplayAbility* Instance : Spec.GetAbilityInstances())
		{
			if (!Instance->IsActive())
			{
				continue;
			}

			const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Instance);
			if (!Ability)
			{
				continue;
			}

			if (Ability->ActivationGroup == EWxAbilityActivationGroup::Exclusive_Blocking || Ability->ActivationGroup == EWxAbilityActivationGroup::Reaction)
			{
				return Ability;
			}
		}
	}

	return nullptr;
}

void UWxAbilitySystemComponent::CancelActivationGroupAbilities(EWxAbilityActivationGroup Group, UGameplayAbility* IgnoreAbility)
{
	// 취소가 어빌리티 목록을 바꿀 수 있으므로 순회를 잠근다.
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
		{
			const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Instance);
			if (Ability && Ability != IgnoreAbility && Ability->IsActive() && Ability->ActivationGroup == Group)
			{
				CancelAbilitySpec(Spec, IgnoreAbility);
				break;
			}
		}
	}
}

void UWxAbilitySystemComponent::HandleHitStopElapsed(TWeakObjectPtr<UAnimMontage> FrozenMontage)
{
	// 피격 등이 현재 몽타주를 가로챘어도 얼렸던 그 몽타주에 정확히 닿는다.
	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (AnimInstance && FrozenMontage.IsValid())
	{
		AnimInstance->Montage_SetPlayRate(FrozenMontage.Get(), GetMontagePlayRate());
	}
}
