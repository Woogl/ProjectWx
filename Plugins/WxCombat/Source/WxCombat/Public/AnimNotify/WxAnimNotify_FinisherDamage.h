// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_FinisherDamage.generated.h"

/**
 * 몽타주를 재생 중인 피니셔 어빌리티가 상호작용으로 확정해 둔 대상에 이 프레임의 대미지를 적용한다.
 * 앞잡·뒤잡 구분 없이 DamageDataRow의 계수를 쓴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;
};
