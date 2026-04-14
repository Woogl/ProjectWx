// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_WeaponAttack.h"
#include "WxDamageTableRow.h"
#include "Weapon/WxWeaponBase.h"

UWxAnimNotifyState_WeaponAttack::UWxAnimNotifyState_WeaponAttack()
{
	// 애님 평가 내부에서 동기 실행되어 저프레임/히치 상황에서도 히트 구간이 스킵되지 않고,
	// 콤보 전환 시 이전/다음 ANS의 Begin/End 순서가 애님 시간 기준으로 보장된다.
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
		Weapon->BeginAttack(ResolveDamageInfo());
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

FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Attack");
}

FWxDamageInfo UWxAnimNotifyState_WeaponAttack::ResolveDamageInfo() const
{
	if (const FWxDamageTableRow* Row = DamageDataRow.GetRow<FWxDamageTableRow>(TEXT("WxAnimNotifyState_WeaponAttack")))
	{
		FWxDamageInfo Resolved;
		Resolved.ApplyTableRow(*Row);
		return Resolved;
	}

	return DamageInfo;
}
