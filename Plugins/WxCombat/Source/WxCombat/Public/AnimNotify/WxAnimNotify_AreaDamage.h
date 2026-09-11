// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_AreaDamage.generated.h"

class UTargetingPreset;

/**
 * TargetingPreset이 고른 액터 전원에게 한 시점에 대미지 행을 적용한다. 범위·형상·필터는 전부 프리셋이 저작한다.
 * 쿼리는 동기 진입점으로 돌려 그 자리에서 결과를 읽는다. 비동기 요청으로 바꾸면 결과가 다음 프레임에 도착해 대미지가 사라진다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_AreaDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	/** 프리셋의 AOE 범위를 노티파이 색으로 그린다. 표시 구간과 프레임 규칙은 WxTargetingPreview가 갖는다. */
	virtual void DrawInEditor(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp, const UAnimSequenceBase* Animation, const FAnimNotifyEvent& NotifyEvent) const override;
#endif

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle DamageDataRow;
};
