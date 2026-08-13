// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// 항상 서버에서 발행된 GameplayEvent로 트리거된다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_HitReact);
	SetAssetTags(AssetTags);

	// 피격은 반응이 끝날 때까지 새 액션을 막는다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	// 진행 중인 것은 공격·스킬만 끊는다 — 마커로 끊으면 마커를 가진 적 패턴까지 평타 피격에 중단된다.
	// 차단만으로는 부족하다: 공격·스킬의 콤보 재발동 분기는 활성 Spec만 보고 자체 판정하므로 ASC의 차단 태그 검사를 건너뛴다.
	// Ability.Skill은 부모 태그라 슬롯별 Ability.Skill.1~4까지 함께 잡는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_HitReact);

	// Ability.Death 차단이 사망 몽타주를 지킨다 — 대미지 연출은 사망 처리 뒤에 발행된다.
	// 풀면 히트리액트가 사망 몽타주를 끊어 래그돌 폴백으로 떨어뜨린다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Guard);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_SuperArmor);

	bRetriggerInstancedAbility = true;

	// GameplayEvent 트리거는 부모 체인을 거슬러 조회되므로 부모 하나로 Event.HitReact.* 전부를 받는다.
	// 자식을 함께 등록하면 조상마다 한 번씩 발화해 같은 피격에 몽타주가 재시작한다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 회전·띄우기보다 먼저 판정한다 — 커밋이 막힌 뒤에 연출만 남으면 캐릭터가 어빌리티 없이 공중에 뜬다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayTag EventTag = TriggerEventData ? TriggerEventData->EventTag : WxGameplayTags::Event_HitReact_Normal;
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	UAnimMontage* SelectedMontage = nullptr;
	if (EventTag == WxGameplayTags::Event_HitReact_KnockBack)
	{
		SelectedMontage = KnockbackMontage;
		FaceInstigator(AvatarActor, Instigator);
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_KnockDown)
	{
		SelectedMontage = KnockdownMontage;
		FaceInstigator(AvatarActor, Instigator);
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_KnockUp)
	{
		SelectedMontage = KnockupMontage;

		if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
		{
			const FVector LaunchVelocity(0.f, 0.f, Character->GetCharacterMovement()->JumpZVelocity);
			Character->LaunchCharacter(LaunchVelocity, false, true);
		}
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_Parry)
	{
		SelectedMontage = ParryReactMontage;
		FaceInstigator(AvatarActor, Instigator);
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_Finisher)
	{
		SelectedMontage = FinisherHitReactMontage;
		FaceInstigator(AvatarActor, Instigator);
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_Backstab)
	{
		SelectedMontage = BackstabHitReactMontage;
		FaceInstigator(AvatarActor, Instigator);
	}

	// 종류별 몽타주를 지정하지 않았으면 일반 피격으로 대체한다 — 반응 자체는 이벤트가 정하므로 위에서 이미 적용됐다.
	if (!SelectedMontage)
	{
		SelectedMontage = NormalHitReactMontage;
	}

	if (!SelectedMontage || !PlayHitReactMontage(SelectedMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

bool UWxAbility_HitReact::PlayHitReactMontage(UAnimMontage* Montage)
{
	// ASPD를 반영하지 않는다 — 처형 짝 피격이 공격자 몽타주와 프레임 싱크돼야 한다.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	return true;
}

void UWxAbility_HitReact::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_HitReact::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_HitReact::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_HitReact::FaceInstigator(AActor* AvatarActor, const AActor* Instigator)
{
	if (!AvatarActor || !Instigator)
	{
		return;
	}

	FVector Direction = Instigator->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0;
	if (!Direction.IsNearlyZero())
	{
		AvatarActor->SetActorRotation(Direction.ToOrientationRotator());
	}
}
