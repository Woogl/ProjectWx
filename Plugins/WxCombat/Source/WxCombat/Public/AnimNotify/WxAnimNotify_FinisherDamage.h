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
 * 처형 공격 몽타주에 배치하면 해당 프레임에서, 몽타주를 재생 중인 피니셔 어빌리티가 상호작용으로
 * 확정한 대상에 대미지를 적용한다. 대미지 수치는 이 노티파이가 대미지 테이블 행으로 직접 입력한다.
 * (어빌리티는 대상만 쥐고 적용을 수행하므로 앞잡·뒤잡이 동일 경로로 통일된다.)
 *
 * 어빌리티는 ServerInitiated 라 서버 인스턴스에서만 활성화된다. 노티파이는 클라/서버 양쪽에서
 * 발화하지만, 클라에서는 GetAnimatingAbility 캐스팅이 실패해 무동작하고 서버에서만 적용된다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

protected:
	/** 설정 시 DamageInfo를 테이블에서 읽어 사용한다. 미설정 시 아래 DamageInfo를 직접 사용 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ShowOnlyInnerProperties))
	FWxDamageInfo DamageInfo;

private:
	FWxDamageInfo ResolveDamageInfo() const;
};
