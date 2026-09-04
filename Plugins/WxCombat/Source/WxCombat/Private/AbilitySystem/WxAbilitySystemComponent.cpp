// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxCombatModule.h"
#include "Components/SkeletalMeshComponent.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

float UWxAbilitySystemComponent::PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName, float StartTimeSeconds)
{
	const float Duration = Super::PlayMontage(AnimatingAbility, ActivationInfo, Montage, InPlayRate, StartSectionName, StartTimeSeconds);
	if (Duration > 0.f && AnimatingAbility != nullptr)
	{
		EnableAnimatingMontageMeshTick();
	}

	return Duration;
}

void UWxAbilitySystemComponent::ClearAnimatingAbility(UGameplayAbility* Ability)
{
	const bool bClearingCurrentAbility = IsAnimatingAbility(Ability);
	Super::ClearAnimatingAbility(Ability);

	if (bClearingCurrentAbility && GetAnimatingAbility() == nullptr)
	{
		RestoreAnimatingMontageMeshTick();
	}
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

void UWxAbilitySystemComponent::EnableAnimatingMontageMeshTick()
{
	if (MontageTickMesh.IsValid())
	{
		return;
	}

	USkeletalMeshComponent* Mesh = AbilityActorInfo.IsValid() ? AbilityActorInfo->SkeletalMeshComponent.Get() : nullptr;
	if (Mesh == nullptr)
	{
		return;
	}

	PreviousMontageTickOption = Mesh->VisibilityBasedAnimTickOption;
	MontageTickMesh = Mesh;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	UE_LOG(LogWxCombat, Verbose, TEXT("Montage mesh tick enabled: Mesh=%s, Ability=%s"), *GetNameSafe(Mesh), *GetNameSafe(GetAnimatingAbility()));
}

void UWxAbilitySystemComponent::RestoreAnimatingMontageMeshTick()
{
	USkeletalMeshComponent* Mesh = MontageTickMesh.Get();
	MontageTickMesh.Reset();
	if (Mesh == nullptr)
	{
		return;
	}

	Mesh->VisibilityBasedAnimTickOption = PreviousMontageTickOption;
	UE_LOG(LogWxCombat, Verbose, TEXT("Montage mesh tick restored: Mesh=%s"), *GetNameSafe(Mesh));
}

bool UWxAbilitySystemComponent::AbilityInputActionTriggered(const UInputAction* Action)
{
	if (!Action)
	{
		return false;
	}

	// 순회 중 활성화가 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, RemoveAfterActivation 등).
	// 락이 없으면 Give/Clear가 즉시 Add/RemoveAtSwap 해 참조와 이터레이터가 무효화된다.
	ABILITYLIST_SCOPE_LOCK();

	// 순정 AbilityLocalInputPressed처럼 활성 여부와 무관하게 키 상태를 스펙에 남긴다.
	// 홀드 어빌리티가 발동 조건으로 읽는다.
	// 아래 발동 순회는 첫 성립에서 입력을 소비하고 끊기므로, 세우기를 먼저 끝내야 한 IA를 공유하는 뒤 스펙도 내리기와 대칭이 된다.
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (Ability && Ability->ActivationInputAction.Get() == Action)
		{
			Spec.InputPressed = true;
		}
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
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
	}

	return false;
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

bool UWxAbilitySystemComponent::TryActivateByInputAction(const UInputAction* Action)
{
	// AbilityInputActionTriggered와 같은 이유로 락을 건다.
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

void UWxAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	UE_LOG(LogWxCombat, Verbose, TEXT("Ability Failed: %s — 사유 %s"), *GetNameSafe(Ability), *FailureReason.ToStringSimple());
}

void UWxAbilitySystemComponent::CancelRecoveringAbilities(UGameplayAbility* IgnoreAbility)
{
	// 취소가 어빌리티 목록을 바꿀 수 있으므로 순회를 잠근다.
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		// FindActivationGroupBlocker와 같은 근거 — 기반 생성자가 InstancedPerActor를 강제하므로 스펙당 인스턴스는 하나뿐이다.
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetPrimaryInstance());
		if (Ability && Ability != IgnoreAbility && Ability->IsActive() && Ability->ActivationGroup == EWxAbilityActivationGroup::Exclusive && Ability->ActionPhase == EWxAbilityActionPhase::Recovery)
		{
			CancelAbilitySpec(Spec, IgnoreAbility);
		}
	}
}
