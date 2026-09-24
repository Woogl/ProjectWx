// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Death.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "WxGameplayTags.h"

UWxAbility_Death::UWxAbility_Death()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 사망은 되돌릴 수 없다 — 클라가 보내는 실행·종료 요청을 서버가 무시한다.
	// Override 그룹이라 취소는 이미 거부되지만, 종료 요청은 CanBeCanceled를 거치지 않아 그대로 통과한다.
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Death);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Death);
	
	// 이후 발동은 전부 막고, 진행 중인 것은 액션만 끊는다 — 반응은 취소되지 않는다.
	// 활성 반응은 사망 몽타주가 밀어내며 끝나고, 그로기는 Ability.Death를 직접 보고 스스로 끝난다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	
	ActivationGroup = EWxAbilityActivationGroup::Override;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Death;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

float UWxAbility_Death::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 커밋하지 않는다 — 사망은 코스트·쿨다운이 없는 강제 전이이고, 커밋 실패가 곧 사망 미성립(Ability.Death 미부여)이 된다.

	// 시체의 피격 판정 해제와 수명은 AWxCharacterBase::HandleDeath가, BT 정지는 그 OnDeath를 받는 AI 컨트롤러가 맡는다 — 모두 발동 전에 붙는 Ability.Death로 이미 끝나 있다.
	PlayDeathMontageOrRagdoll();
}

void UWxAbility_Death::HandleMontageCompleted()
{
}

void UWxAbility_Death::HandleMontageInterrupted()
{
	EnableRagdoll();
}

void UWxAbility_Death::HandleMontageCancelled()
{
	EnableRagdoll();
}

void UWxAbility_Death::PlayDeathMontageOrRagdoll()
{
	// HitReact 등 활성 몽타주는 PlayMontageAndWait가 BlendOut으로 인계받는다.
	if (!PlayMontage(DeathMontage))
	{
		EnableRagdoll();
		return;
	}

	// 포즈를 유지하는 사망 몽타주는 끝에 닿아도 완료 통지가 오지 않아 재생 길이로 끝을 잡는다.
	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, DeathMontage->GetPlayLength() / (GetMontagePlayRate() * DeathMontage->RateScale));
	WaitTask->OnFinish.AddDynamic(this, &UWxAbility_Death::HandleDeathMontageElapsed);
	WaitTask->ReadyForActivation();
}

void UWxAbility_Death::HandleDeathMontageElapsed()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ClearAnimatingAbility(this);
	}
}

void UWxAbility_Death::EnableRagdoll()
{
	// 오너 클라 인스턴스도 이 경로에 들어오므로, 루스 태그가 중복 추가되지 않게 서버에서만 발행한다.
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Ragdoll, 1, EGameplayTagReplicationState::TagOnly);
	}
}
