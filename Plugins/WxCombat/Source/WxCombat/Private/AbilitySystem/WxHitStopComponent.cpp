// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxHitStopComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

UWxHitStopComponent::UWxHitStopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxHitStopComponent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent = Cast<UWxAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxHitStopComponent::HandleActiveGameplayEffectAdded);
		AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxHitStopComponent::HandleGameplayEffectRemoved);
	}
}

void UWxHitStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UWxHitStopComponent::HandleActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!Spec.Def || !Spec.Def->GetGrantedTags().HasTag(WxGameplayTags::Effect_HitStop))
	{
		return;
	}

	const FGameplayEffectContextHandle& Context = Spec.GetContext();
	if (Context.GetInstigatorAbilitySystemComponent() == AbilitySystemComponent)
	{
		if (!AbilitySystemComponent->GetAnimatingAbility() || AbilitySystemComponent->GetAnimatingAbility() != Context.GetAbilityInstance_NotReplicated())
		{
			return;
		}
	}
	else
	{
		const bool bLocallyControlled = AbilitySystemComponent->AbilityActorInfo.IsValid() && AbilitySystemComponent->AbilityActorInfo->IsLocallyControlled();
		if (!AbilitySystemComponent->IsOwnerActorAuthoritative() && !bLocallyControlled)
		{
			return;
		}
	}

	UAnimMontage* Montage = AbilitySystemComponent->GetCurrentMontage();
	UAnimInstance* AnimInstance = AbilitySystemComponent->AbilityActorInfo.IsValid() ? AbilitySystemComponent->AbilityActorInfo->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance)
	{
		return;
	}

	// 완전한 0이 아닌 미세 값으로 둬 몽타주 진행 판정 이슈를 피한다.
	// 복원과 같이 AnimInstance에 직접 건다 — CurrentMontageSetPlayRate는 클라에서 서버로 RPC를 보내, 서버가 이미 되돌린 몽타주를 뒤늦게 다시 얼릴 수 있다. 서버 쪽 값은 매 틱 몽타주 복제 데이터로 동기화된다.
	AnimInstance->Montage_SetPlayRate(Montage, 0.001f);
	FrozenHandles.Add(Handle);
}

void UWxHitStopComponent::HandleGameplayEffectRemoved(const FActiveGameplayEffect& RemovedEffect)
{
	if (FrozenHandles.Remove(RemovedEffect.Handle) == 0 || !FrozenHandles.IsEmpty())
	{
		return;
	}

	// 얼린 뒤 몽타주가 바뀌었어도(피격 등) 새 몽타주는 이미 제 배속이라 지금 것을 지금 주인의 배속으로 되돌리면 된다.
	UAnimMontage* Montage = AbilitySystemComponent->GetCurrentMontage();
	UAnimInstance* AnimInstance = AbilitySystemComponent->AbilityActorInfo.IsValid() ? AbilitySystemComponent->AbilityActorInfo->GetAnimInstance() : nullptr;
	if (!Montage || !AnimInstance)
	{
		return;
	}

	const UWxAbilityBase* MontageAbility = Cast<UWxAbilityBase>(AbilitySystemComponent->GetAnimatingAbility());
	AnimInstance->Montage_SetPlayRate(Montage, MontageAbility ? MontageAbility->GetMontagePlayRate() : AbilitySystemComponent->GetMontagePlayRate());
}
