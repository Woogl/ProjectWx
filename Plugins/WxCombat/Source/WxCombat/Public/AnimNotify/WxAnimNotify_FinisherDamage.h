// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxDamageInfo.h"
#include "WxAnimNotify_FinisherDamage.generated.h"

/**
 * 처형(앞잡·뒤잡) 대미지 AnimNotify.
 *
 * 처형 공격 몽타주에 배치하면 해당 프레임에서, 몽타주를 재생 중인 피니셔 어빌리티가 상호작용으로 확정한 대상에 대미지를 적용한다.
 * 무엇을 적용할지는 어빌리티가 현재 재생 몽타주로 정한다: 뒤잡은 즉사 GE, 앞잡은 아래 DamageDataRow의 계수 대미지.
 * 노티파이는 적용 타이밍만 잡고, 앞잡 계수 수치를 DamageDataRow로 넘긴다.
 *
 * 어빌리티는 ServerInitiated 라 서버 인스턴스에서만 활성화된다.
 * 노티파이는 클라/서버 양쪽에서 발화하지만, 클라에서는 GetAnimatingAbility 캐스팅이 실패해 무동작하고 서버에서만 적용된다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 앞잡 계수 대미지 수치를 읽을 대미지 테이블 행. (뒤잡은 어빌리티가 즉사 GE를 적용하므로 무시된다) */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;
};
