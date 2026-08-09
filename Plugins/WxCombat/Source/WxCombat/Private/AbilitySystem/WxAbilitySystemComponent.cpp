// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"
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

		// 재발동을 받지 않는 어빌리티는 활성 인스턴스가 입력을 직접 처리한다.
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

int32 UWxAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (EventTag == WxGameplayTags::Event_HitStop && Payload)
	{
		ApplyHitStop(*Payload);
	}

	return Super::HandleGameplayEvent(EventTag, Payload);
}

void UWxAbilitySystemComponent::ApplyHitStop(const FGameplayEventData& Payload)
{
	// SetTimer가 0 이하를 예약 취소로 취급하므로, 그대로 두면 복원 없는 정지가 된다.
	const float Duration = Payload.EventMagnitude;
	if (Duration <= 0.f)
	{
		return;
	}

	// 대미지를 준 그 어빌리티가 여전히 몽타주 주인일 때만 얼린다.
	// 같은 적중 처리에서 먼저 발동한 반응(패리 등)이 몽타주를 가로챘으면 건너뛴다.
	if (!GetAnimatingAbility() || GetAnimatingAbility() != Payload.ContextHandle.GetAbilityInstance_NotReplicated())
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

	// 복원 예약이 지금 얼린 그 몽타주를 들고 간다.
	// 연속 적중이면 타이머가 재설정되어 조기 복원을 막는다.
	GetWorld()->GetTimerManager().SetTimer(HitStopResumeTimer,
		FTimerDelegate::CreateUObject(this, &UWxAbilitySystemComponent::HandleHitStopElapsed, TWeakObjectPtr<UAnimMontage>(Montage)),
		Duration, false);
}

void UWxAbilitySystemComponent::HandleHitStopElapsed(TWeakObjectPtr<UAnimMontage> FrozenMontage)
{
	// 복원은 현재 몽타주가 아니라 얼렸던 그 몽타주에 간다.
	// 피격 등이 현재를 가로챘어도 정확히 닿고, 인스턴스가 사라졌으면 무동작이다.
	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (AnimInstance && FrozenMontage.IsValid())
	{
		AnimInstance->Montage_SetPlayRate(FrozenMontage.Get(), GetMontagePlayRate());
	}
}
