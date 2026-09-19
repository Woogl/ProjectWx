// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "System/WxCombatDeveloperSettings.h"
#include "WxCombatLibrary.h"

FLinearColor UWxAnimNotifyState_ApplyGameplayEffect::GetEditorColor()
{
	return GetDefault<UWxCombatDeveloperSettings>()->CombatAnimNotifyColor;
}

void UWxAnimNotifyState_ApplyGameplayEffect::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	// 비동기 노티파이에서는 활성화 예측 키를 재사용하지 않고 서버 GE의 복제를 따른다.
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
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
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	// 일치하는 각 GE에서 스택 하나를 제거한다. GE 인스턴스 하나만 선택하는 인자는 아니다.
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
