// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxDamageInfo.h"
#include "WxAnimNotify_FinisherDamage.generated.h"

/**
 * 처형(앞잡·뒤잡) 대미지 AnimNotify.
 * 몽타주를 재생 중인 피니셔 어빌리티가 상호작용으로 확정해 둔 대상에 이 프레임의 대미지를 적용한다.
 * 앞잡·뒤잡 구분 없이 DamageDataRow의 계수를 쓰므로, 타이밍도 수치도 이 노티파이가 소유한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 앞잡·뒤잡 몽타주가 같은 행을 가리키면 두 처형의 대미지가 같아진다 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;
};
