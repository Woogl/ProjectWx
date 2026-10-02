// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "AbilitySystem/Attributes/WxCombatAttributeSet.h"
#include "AbilitySystem/Effects/WxEffect_Exhaust.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "WxGame.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxGameplayTags.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWxAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	FOnGameplayAttributeValueChange& SPChanged = GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetSPAttribute());
	if (!SPChanged.IsBoundToObject(this))
	{
		SPChanged.AddUObject(this, &UWxAbilitySystemComponent::HandleSPChanged);
	}
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

void UWxAbilitySystemComponent::MulticastStopMontage_Implementation(UAnimMontage* Montage)
{
	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance)
	{
		return;
	}

	if (GetCurrentMontage() == Montage)
	{
		// 추적 중인 몽타주는 GAS의 복제 상태도 정지시켜 늦은 복제로 재생되지 않게 한다.
		CurrentMontageStop(Montage->GetDefaultBlendOutTime());
	}
	else
	{
		AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
	}
}

void UWxAbilitySystemComponent::GiveAbilitySets()
{
	if (!IsRegistered() || !IsOwnerActorAuthoritative())
	{
		return;
	}

	const bool bInitialize = !bAbilitySetsInitialized;
	// 재등록으로 어빌리티가 지워져도 기존 속성과 GE는 남는다.
	bAbilitySetsInitialized = true;

	for (const TObjectPtr<UWxAbilitySet>& Set : AbilitySets)
	{
		if (Set)
		{
			if (bInitialize)
			{
				Set->GiveToAbilitySystem(this);
			}
			else
			{
				Set->GiveAbilitiesToAbilitySystem(this);
			}
		}
	}
}

void UWxAbilitySystemComponent::HandleSPChanged(const FOnAttributeChangeData& ChangeData)
{
	// 클라도 복제 수신으로 이 콜백을 지난다.
	if (ChangeData.NewValue >= ChangeData.OldValue || !IsOwnerActorAuthoritative())
	{
		return;
	}

	if (GetNumericAttribute(UWxCombatAttributeSet::GetMaxSPAttribute()) > 0.f)
	{
		UWxEffect_Exhaust::ApplyTo(this);
	}
}

void UWxAbilitySystemComponent::EnableAnimatingMontageMeshTick()
{
	if (MontageTickMesh.IsValid())
	{
		return;
	}

	USkeletalMeshComponent* Mesh = AbilityActorInfo.IsValid() ? AbilityActorInfo->SkeletalMeshComponent.Get() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (AnimInstance == nullptr)
	{
		return;
	}

	PreviousMontageTickOption = Mesh->VisibilityBasedAnimTickOption;
	MontageTickMesh = Mesh;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// AnimatingAbility는 슬롯과 무관하게 마지막 어빌리티 하나만 가리켜, 가산 슬롯 피격이 끝나면 아직 재생 중인 몽타주를 두고 비워진다.
	AnimInstance->OnAllMontageInstancesEnded.AddUniqueDynamic(this, &ThisClass::RestoreAnimatingMontageMeshTick);

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

	// 순회 중 활성화가 목록을 바꾸면(GE의 GrantedAbilities, RemoveAfterActivation 등) 락 없이는 Give/Clear가 즉시 Add/RemoveAtSwap 해 참조와 이터레이터가 무효화된다.
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

	if (TryActivateByInputAction(Action))
	{
		return true;
	}

	// 발동 시도 뒤에 알린다. 먼저 알리면 InputPressed로 꺼지는 토글(락온)이 같은 입력에 다시 켜진다.
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (Ability && Ability->ActivationInputAction.Get() == Action && Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);
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

	// 입력을 기다리는 활성 어빌리티(콤보 창)가 같은 IA의 신규 발동보다 먼저 받는다.
	// 홀드 입력은 매 프레임 여기까지 오므로 사본을 만드는 GetAbilityInstances 대신 두 배열을 직접 훑는다.
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability || Ability->ActivationInputAction.Get() != Action || !Spec.IsActive())
		{
			continue;
		}

		for (const UGameplayAbility* Instance : Spec.ReplicatedInstances)
		{
			if (InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey()))
			{
				return true;
			}
		}

		for (const UGameplayAbility* Instance : Spec.NonReplicatedInstances)
		{
			if (InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey()))
			{
				return true;
			}
		}
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (Ability && Ability->ActivationInputAction.Get() == Action && TryActivateAbility(Spec.Handle))
		{
			return true;
		}
	}

	return false;
}

TArray<const UInputAction*> UWxAbilitySystemComponent::GetAbilityInputActions() const
{
	TArray<const UInputAction*> InputActions;
	for (const TObjectPtr<UWxAbilitySet>& Set : AbilitySets)
	{
		if (Set)
		{
			Set->AppendInputActions(InputActions);
		}
	}
	return InputActions;
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

		// 기반 생성자가 InstancedPerActor를 사용하므로 스펙당 인스턴스는 하나뿐이다.
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetPrimaryInstance());
		if (Ability && Ability != IgnoreAbility && Ability->IsActive() && Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Action) && Ability->GetActionPhase() == EWxAbilityActionPhase::Recovery)
		{
			CancelAbilitySpec(Spec, IgnoreAbility);
		}
	}
}

bool UWxAbilitySystemComponent::AreAbilityTagsBlockedIgnoringContribution(const FGameplayTagContainer& AbilityTags, const FGameplayTagContainer& IgnoredBlockTags) const
{
	for (const FGameplayTag& BlockTag : BlockedAbilityTags.GetExplicitGameplayTags())
	{
		// 부모 집계는 자식 차단까지 더하므로, 자기 기여를 뺄 때는 직접 등록된 횟수만 비교한다.
		const int32 IgnoredCount = IgnoredBlockTags.HasTagExact(BlockTag) ? 1 : 0;
		if (AbilityTags.HasTag(BlockTag) && BlockedAbilityTags.GetExplicitTagCount(BlockTag) > IgnoredCount)
		{
			return true;
		}
	}
	return false;
}
