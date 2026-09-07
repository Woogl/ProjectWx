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
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	// 몽타주는 전 머신에 재생되지만 활성 GE 배열의 주인은 서버다 — 비권위 머신의 제거는 예측이 아니라 복제본 삭제라, 서버 항목이 그대로면 그 클라만 태그가 어긋난 채 남는다.
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	// 수량을 1로 잡아야 회피가 캔슬되며 늦게 도착한 이 호출이 이미 걸린 처형 무적까지 벗기지 않는다.
	ASC->RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1);
}

FString UWxAnimNotifyState_ApplyGameplayEffect::GetNotifyName_Implementation() const
{
	if (EffectClass)
	{
		return EffectClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
