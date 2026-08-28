// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "WxCombatLibrary.h"

void UWxAnimNotifyState_ApplyGameplayEffect::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	UWxCombatLibrary::ApplyEffect(ASC, EffectClass, ASC->GetAnimatingAbility());
}

void UWxAnimNotifyState_ApplyGameplayEffect::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		// 수량을 1로 잡아야 회피가 캔슬되며 늦게 도착한 이 호출이 이미 걸린 처형 무적까지 벗기지 않는다.
		ASC->RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1);
	}
}

FString UWxAnimNotifyState_ApplyGameplayEffect::GetNotifyName_Implementation() const
{
	if (EffectClass)
	{
		return EffectClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
