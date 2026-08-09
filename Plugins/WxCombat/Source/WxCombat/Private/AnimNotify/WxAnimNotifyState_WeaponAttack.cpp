// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_WeaponAttack.h"
#include "Weapon/WxWeaponBase.h"

UWxAnimNotifyState_WeaponAttack::UWxAnimNotifyState_WeaponAttack()
{
	// 애님 평가 내부에서 동기 실행돼 저프레임·히치에도 히트 구간이 스킵되지 않고, 콤보 전환 시 Begin/End 순서가 애님 시간 기준으로 보장된다.
	bIsNativeBranchingPoint = true;
}

void UWxAnimNotifyState_WeaponAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner))
	{
		Weapon->BeginAttack(FWxDamageInfo::FromDataRow(DamageDataRow));
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

	if (AWxWeaponBase* Weapon = AWxWeaponBase::FindWeapon(Owner))
	{
		Weapon->EndAttack();
	}
}
