// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystem/Task/WxAbilityTask_WaitInputTagPressed.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "WxGameplayTags.h"

UWxAbility_Guard::UWxAbility_Guard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Guard);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	bRetriggerInstancedAbility = true;
}

void UWxAbility_Guard::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// 시퀀스가 자체적으로 종료되어야 하는 페이즈는 입력 릴리즈로 끊지 않는다.
	// - GuardBreak: 가드 깨짐 연출 완주 보장
	// - PerfectGuard: 반격 윈도우 보존 (가드 키를 떼도 ComboWindow 내 공격 입력으로 카운터 가능)
	// - GuardCounter: 반격 연출 완주 보장
	if (ActiveMontage == GuardBreakMontage || ActiveMontage == PerfectGuardMontage || ActiveMontage == GuardCounterMontage)
	{
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Guard::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 패링(PerfectGuard) 모션 중 가드를 재입력하면 후속 연출을 끊고 즉시 가드 자세로 복귀한다.
	// State.Guard는 패링 중에도 유지되므로 GuardMontage만 다시 재생하면 가드가 이어진다.
	if (ActiveMontage == PerfectGuardMontage && GuardMontage)
	{
		PlayMontage(GuardMontage);
	}
}

float UWxAbility_Guard::GetDamageReductionRate() const
{
	return DamageReductionRate;
}

void UWxAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// bRetriggerInstancedAbility로 인해 어빌리티가 진행 중에 재진입되면 현재 페이즈를 유지한다.
	// 이벤트 리스너 중복 등록을 방지하고, GuardBreak 중 재진입 시 태스크를 보호한다.
	if (ActiveMontage)
	{
		return;
	}

	if (!GuardMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	ASC->AddLooseGameplayTag(WxGameplayTags::State_Guard);

	if (!PlayMontage(GuardMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ListenForGuardHit();
	ListenForPerfectGuard();

	// ANS_ComboWindow 구간 공격 입력으로 반격 전환
	if (GuardCounterMontage)
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			Spec->GetDynamicSpecSourceTags().AddTag(WxGameplayTags::Input_Attack);
			ASC->MarkAbilitySpecDirty(*Spec);
		}

		ListenForCounterInput();
	}
}

void UWxAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard))
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
	}

	// ActivateAbility에서 추가한 공격 입력 태그 제거
	if (GuardCounterMontage && ASC)
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			Spec->GetDynamicSpecSourceTags().RemoveTag(WxGameplayTags::Input_Attack);
			ASC->MarkAbilitySpecDirty(*Spec);
		}
	}

	ActiveMontage = nullptr;
	CurrentMontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWxAbility_Guard::PlayMontage(UAnimMontage* Montage)
{
	// 페이즈 전환 시 이전 몽타주 태스크를 명시적으로 정리해 콜백 잔여 발생을 차단한다.
	// HandleMontageBlendingOut 콜백 내에서 호출될 수 있으나, EndTask가 AnimInstance 바인딩을
	// 해제하므로 구 태스크의 OnInterrupted 등 후속 이벤트는 발송되지 않는다.
	if (CurrentMontageTask)
	{
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	CurrentMontageTask = MontageTask;
	ActiveMontage = Montage;

	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Guard::HandleMontageBlendingOut);
	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Guard::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Guard::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Guard::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
	return true;
}

void UWxAbility_Guard::ListenForGuardHit()
{
	// Event.HitReact 부모 태그로 등록하여 자식 태그(.Normal/.Knockback/.Knockdown/.Knockup)를 모두 수신한다.
	// HitReact 어빌리티는 ActivationBlockedTags=State.Guard 로 가드 중에는 활성화되지 않으므로 라우팅 충돌 없음.
	UAbilityTask_WaitGameplayEvent* HitReactTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_HitReact, nullptr, false, false);
	if (HitReactTask)
	{
		HitReactTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandleGuardHitReact);
		HitReactTask->ReadyForActivation();
	}
}

void UWxAbility_Guard::ListenForPerfectGuard()
{
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_PerfectGuard);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandlePerfectGuard);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Guard::ListenForCounterInput()
{
	WaitInputTask = UWxAbilityTask_WaitInputTagPressed::CreateTask(this, WxGameplayTags::Input_Attack);
	if (WaitInputTask)
	{
		WaitInputTask->OnPressed.AddDynamic(this, &UWxAbility_Guard::HandleCounterInputPressed);
		WaitInputTask->ReadyForActivation();
	}
}

void UWxAbility_Guard::PlayGuardCounterMontage()
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	// 반격은 가드 상태가 아니므로 State.Guard 해제
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
		}
	}

	if (!PlayMontage(GuardCounterMontage))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Guard::HandleGuardHitReact(FGameplayEventData Payload)
{
	if (ActiveMontage != GuardMontage)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UWxCombatAttributeSet* AttributeSet = ASC ? ASC->GetSet<UWxCombatAttributeSet>() : nullptr;

	// Payload.EventMagnitude는 ExecCalc가 전달한 SP 차감량.
	// ExecCalc는 SP OutputModifier를 큐잉한 직후 동기적으로 이벤트를 디스패치하므로
	// 이 시점의 GetSP()는 차감 적용 전 값이며, (GetSP() - Magnitude)가 차감 후 예상 SP다.
	const bool bWillBreak = AttributeSet && (AttributeSet->GetSP() - Payload.EventMagnitude) <= 0.f;

	if (bWillBreak)
	{
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
		}

		if (!PlayMontage(GuardBreakMontage))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
	}
	else
	{
		// Knock 계열(Knockback/Knockdown/Knockup)이면 GuardKnockback, 그 외엔 GuardHitReact 재생.
		const bool bIsKnockHit = Payload.EventTag.IsValid() && Payload.EventTag != WxGameplayTags::Event_HitReact_Normal;

		if (bIsKnockHit && GuardKnockbackMontage)
		{
			PlayMontage(GuardKnockbackMontage);
		}
		else if (GuardHitReactMontage)
		{
			PlayMontage(GuardHitReactMontage);
		}
	}
}

void UWxAbility_Guard::HandlePerfectGuard(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 퍼펙트 가드 성공 보상: MP 회복
	UWxEffect_RecoverResource::ApplyTo(ASC, 0.f, PerfectGuardMPRecovery);

	// 퍼펙트 가드 성공 시 짧은 슬로우 타임 연출
	if (UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectGuardSlowTimeDilation, PerfectGuardSlowTimeDuration))
	{
		SlowTimeTask->ReadyForActivation();
	}

	// GuardMontage 페이즈에서만 GuardHitReactMontage를 재생한다.
	// HitReact/Knockback 재생 중 퍼펙트 가드 이벤트가 오면 MP 회복만 처리하고 몽타주는 전환하지 않는다.
	if (ActiveMontage == GuardMontage && PerfectGuardMontage)
	{
		PlayMontage(PerfectGuardMontage);
	}
}

void UWxAbility_Guard::HandleCounterInputPressed()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// ANS_ComboWindow 구간 내에서만 반격 허용
	if (!ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		ListenForCounterInput();
		return;
	}

	PlayGuardCounterMontage();
}

void UWxAbility_Guard::HandleMontageBlendingOut()
{
	if (ActiveMontage == GuardHitReactMontage || ActiveMontage == GuardKnockbackMontage)
	{
		PlayMontage(GuardMontage);
	}
}

void UWxAbility_Guard::HandleMontageCompleted()
{
	// GuardMontage는 루핑 몽타주이므로 OnCompleted가 발생하지 않는다.
	if (ActiveMontage == GuardMontage)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Guard::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Guard::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
