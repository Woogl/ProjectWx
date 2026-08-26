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
	// ASC는 캐릭터 서브오브젝트라 재빙의 후에도 앞서 부여한 어빌리티를 그대로 쥐고 있다.
	// 다시 부여하면 어빌리티·GE가 중복되고 어트리뷰트 초기화가 HP/SP를 초기값으로 되돌린다.
	if (!AbilitySet || bAbilitySetGranted)
	{
		return;
	}

	bAbilitySetGranted = true;

	AbilitySet->GiveToAbilitySystem(this);
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

	bool bBuffer = false;
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability || Ability->ActivationInputAction.Get() != Action)
		{
			continue;
		}

		// 순정 AbilityLocalInputPressed처럼 활성 여부와 무관하게 키 상태를 스펙에 남긴다.
		// 홀드 어빌리티가 발동 조건으로 읽으며, 버퍼 재생은 뗀 뒤라 이 값을 세우지 않는다.
		Spec.InputPressed = true;

		// 신규 발동과 콤보 재발동은 엔진이 bRetriggerInstancedAbility로 가르므로 호출이 같다.
		if (TryActivateAbility(Spec.Handle))
		{
			// 발동이 성립했으면 쌓아 둔 입력은 전부 낡은 것이다 — 남겨 두면 같은 입력이 라이브와 재생으로 두 번 나간다.
			BufferedInputs.Reset();
			return;
		}

		if (Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);

			// 홀드 입력은 매 프레임 여기까지 오므로 사본을 만드는 GetAbilityInstances 대신 두 배열을 직접 훑는다.
			for (const UGameplayAbility* Instance : Spec.ReplicatedInstances)
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}

			for (const UGameplayAbility* Instance : Spec.NonReplicatedInstances)
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}

		// 배타 게이트의 대상이 아닌 Independent(질주·락온)는 거절 주체가 액션이 아니므로 기억하지 않는다 — 락온 해제 입력이 뒤늦게 다시 켜지는 것도 여기서 막힌다.
		bBuffer = bBuffer || Ability->ActivationGroup != EWxAbilityActivationGroup::Independent;
	}

	// 입력을 거절한 것이 진행 중인 액션(배타 점유자)일 때만 기억한다. 유휴 중 실패(쿨다운·코스트)는 기억할 가치가 없다.
	if (!bBuffer || FindActivationGroupBlockers().IsEmpty())
	{
		return;
	}

	const double Now = GetWorld()->GetRealTimeSeconds();
	for (FWxBufferedInput& Buffered : BufferedInputs)
	{
		// 홀드 중엔 매 프레임 갱신되므로 나이는 사실상 뗀 뒤부터 센다.
		if (Buffered.Action == Action)
		{
			Buffered.TriggeredTime = Now;
			return;
		}
	}

	BufferedInputs.Add({Action, Now});
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

		// 눌림과 대칭으로 활성 여부와 무관하게 내린다.
		Spec.InputPressed = false;

		if (Spec.IsActive())
		{
			AbilitySpecInputReleased(Spec);

			for (const UGameplayAbility* Instance : Spec.ReplicatedInstances)
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}

			for (const UGameplayAbility* Instance : Spec.NonReplicatedInstances)
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

void UWxAbilitySystemComponent::FlushBufferedInputs()
{
	const double Now = GetWorld()->GetRealTimeSeconds();
	for (int32 Index = 0; Index < BufferedInputs.Num();)
	{
		if (Now - BufferedInputs[Index].TriggeredTime > InputBufferDuration)
		{
			BufferedInputs.RemoveAt(Index);
			continue;
		}

		// 전이점당 하나만 — 하나가 성립하면 나머지는 낡은 것이다.
		// 실패한 항목은 남긴다. 콤보 창은 자기 재발동만 열리므로, 거기서 버리면 같이 쌓인 회피가 후딜에 못 나간다.
		if (TryActivateByInputAction(BufferedInputs[Index].Action))
		{
			BufferedInputs.Reset();
			return;
		}

		++Index;
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

	return FMath::Max(AttrSet->GetASPD(), 0.001f);
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

TArray<const UWxAbilityBase*, TInlineAllocator<4>> UWxAbilitySystemComponent::FindActivationGroupBlockers() const
{
	TArray<const UWxAbilityBase*, TInlineAllocator<4>> Blockers;

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		// GetAbilityInstances는 호출마다 두 배열을 합친 사본을 만든다. 인스턴스는 복제 여부로만 갈리므로 각각 훑는다.
		for (const UGameplayAbility* Instance : Spec.ReplicatedInstances)
		{
			if (const UWxAbilityBase* Blocker = AsActivationGroupBlocker(Instance))
			{
				Blockers.Add(Blocker);
			}
		}

		for (const UGameplayAbility* Instance : Spec.NonReplicatedInstances)
		{
			if (const UWxAbilityBase* Blocker = AsActivationGroupBlocker(Instance))
			{
				Blockers.Add(Blocker);
			}
		}
	}

	return Blockers;
}

const UWxAbilityBase* UWxAbilitySystemComponent::AsActivationGroupBlocker(const UGameplayAbility* Instance) const
{
	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Instance);
	if (!Ability || !Ability->IsActive())
	{
		return nullptr;
	}

	if (Ability->ActivationGroup == EWxAbilityActivationGroup::Exclusive_Blocking || Ability->ActivationGroup == EWxAbilityActivationGroup::Exclusive_ComboWindow || Ability->ActivationGroup == EWxAbilityActivationGroup::Reaction)
	{
		return Ability;
	}

	return nullptr;
}

bool UWxAbilitySystemComponent::TryActivateByInputAction(const UInputAction* Action)
{
	// 순회 중 활성화가 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, RemoveAfterActivation 등).
	// 락이 없으면 Give/Clear가 즉시 Add/RemoveAtSwap 해 참조와 이터레이터가 무효화된다.
	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability || Ability->ActivationInputAction.Get() != Action)
		{
			continue;
		}

		// 신규 발동과 콤보 재발동은 엔진이 bRetriggerInstancedAbility로 가르므로 호출이 같다.
		if (TryActivateAbility(Spec.Handle))
		{
			return true;
		}
	}

	return false;
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
