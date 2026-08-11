// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Damage/WxCombatEffectContext.h"
#include "WxGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);

	// InitAbilityActorInfo는 여러 번 불려 중복 바인딩이 되므로 생성자에서 한 번만 건다.
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UWxAbilitySystemComponent::HandleGameplayEffectAppliedToSelf);
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

void UWxAbilitySystemComponent::ApplyHitStop(float Duration, const UGameplayAbility* SourceAbility)
{
	// SetTimer가 0 이하를 예약 취소로 취급하므로, 그대로 두면 복원 없는 정지가 된다.
	if (Duration <= 0.f)
	{
		return;
	}

	// 같은 적중 처리에서 먼저 발동한 반응(패리 등)이 몽타주를 가로챘으면 건너뛴다.
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

void UWxAbilitySystemComponent::HandleGameplayEffectAppliedToSelf(UAbilitySystemComponent* SourceASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	// 컨텍스트가 대미지 GE와 AdditionalEffects에 공유되므로, GE 종류로 걸러야 뒤따르는 상태이상 GE마다 연출이 다시 나가지 않는다.
	if (!Spec.Def || !Spec.Def->IsA<UWxEffect_Damage>())
	{
		return;
	}

	const FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
	const FGameplayEffectContext* RawContext = ContextHandle.Get();
	if (!RawContext || RawContext->GetScriptStruct() != FWxCombatEffectContext::StaticStruct())
	{
		return;
	}

	const FWxCombatEffectContext& CombatContext = static_cast<const FWxCombatEffectContext&>(*RawContext);
	const EWxDamageResult DamageResult = CombatContext.GetDamageResult();

	AActor* TargetActor = GetOwnerActor();
	AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;

	FGameplayEventData EventData;
	EventData.Instigator = SourceActor;
	EventData.Target = TargetActor;

	// 무적 회피는 대미지도 히트스톱도 없다 — 회피 성공만 알린다.
	if (DamageResult == EWxDamageResult::Evaded)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_DodgeSuccess, EventData);
		return;
	}

	// 판정이 없으면 낼 연출이 없다 — 팀에 걸린 아군 히트, ExecCalc를 건너뛴 클라이언트 예측 경로가 그렇다.
	if (DamageResult == EWxDamageResult::None)
	{
		return;
	}

	FVector HitLocation = FVector::ZeroVector;
	if (const FHitResult* HitResult = ContextHandle.GetHitResult())
	{
		HitLocation = FVector(HitResult->ImpactPoint);
	}
	else if (const AActor* TargetAvatar = GetAvatarActor())
	{
		HitLocation = TargetAvatar->GetActorLocation();
	}

	FGameplayCueParameters CueParams;
	CueParams.Location = HitLocation;
	CueParams.EffectContext = ContextHandle;

	if (DamageResult == EWxDamageResult::PerfectGuard)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_PerfectGuard, EventData);

		if (SourceActor && Spec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_ParryHitReact))
		{
			FGameplayEventData ParryEventData;
			ParryEventData.Instigator = TargetActor;
			ParryEventData.Target = SourceActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceActor, WxGameplayTags::Event_HitReact_Parry, ParryEventData);
		}

		ExecuteGameplayCue(WxGameplayTags::GameplayCue_PerfectGuard, CueParams);
	}
	else
	{
		const float FinalDamage = CombatContext.GetFinalDamage();

		// 죽는 히트는 HP 차감이 먼저라 State.Dead가 이미 붙어 있고, HitReact 어빌리티가 그 태그로 차단된다.
		// 히트리액트를 재생했다 사망으로 끊는 대신 곧장 사망으로 가는 쪽을 택했다.
		if (CombatContext.GetHitReactTag().IsValid())
		{
			EventData.EventMagnitude = FinalDamage;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, CombatContext.GetHitReactTag(), EventData);
		}

		if (FinalDamage > 0.f)
		{
			CueParams.RawMagnitude = FinalDamage;

			if (CombatContext.IsCritical())
			{
				CueParams.AggregatedSourceTags.AddTag(WxGameplayTags::Damage_Critical);
			}

			ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
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
