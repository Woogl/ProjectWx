// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_WeaponAttack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "WxGameplayTags.h"

UWxAnimNotifyState_WeaponAttack::UWxAnimNotifyState_WeaponAttack()
{
	HitReactTag = WxGameplayTags::Event_HitReact_Normal;
}

void UWxAnimNotifyState_WeaponAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner);
	if (!Weapon)
	{
		return;
	}

	// AttackTags를 콜리전 활성화보다 먼저 설정한다.
	// SetCollisionEnabled 시 이미 겹쳐있는 액터에 대해 Overlap이 즉시 발생할 수 있으므로,
	// 그 전에 태그가 준비되어 있어야 한다.
	if (bUnblockable)
	{
		Weapon->AttackTags.AddTag(WxGameplayTags::Damage_Unblockable);
	}

	if (HitReactTag.IsValid())
	{
		Weapon->AttackTags.AddTag(HitReactTag);
	}

	Weapon->ATKCoeff = ATKCoeff;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
	}
}

void UWxAnimNotifyState_WeaponAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
	}

	// AttackTags 및 ATKCoeff의 개별 정리를 여기서 수행하지 않는다.
	// FGameplayTagContainer는 참조 카운팅을 하지 않으므로, 연속 공격 콤보에서
	// 다음 ANS의 NotifyBegin과 이전 ANS의 NotifyEnd가 경합할 때
	// 아직 활성인 ANS의 태그까지 조기 제거되는 문제가 발생한다.
	// 모든 ANS가 종료되면 ANS_WeaponCollision 태그 카운트가 0이 되어
	// SetWeaponCollisionEnabled(false)에서 무기 상태가 일괄 초기화된다.
}

FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Attack");
}
