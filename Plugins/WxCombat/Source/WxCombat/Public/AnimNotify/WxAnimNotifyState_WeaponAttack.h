// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxDamageInfo.h"
#include "WxAnimNotifyState_WeaponAttack.generated.h"

/**
 * 무기 공격 구간 AnimNotifyState.
 *
 * NotifyBegin에서 소유자의 무기를 찾아 DamageDataRow로 만든 DamageInfo와 함께 공격 구간을 열고, NotifyEnd에서 닫는다.
 * 히트 콜리전을 켜고 끄는 것은 무기 자신이다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_WeaponAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UWxAnimNotifyState_WeaponAttack();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 이 공격의 대미지 수치 테이블 Row */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;
};
