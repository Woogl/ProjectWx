// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_FinisherDamage.generated.h"

/**
 * 처형 피해 적용 시점에 GameplayEvent를 발행한다.
 * 자신을 페이로드로 전달할 뿐, 피해 적용은 대상을 쥔 UWxAbility_Finisher에 맡긴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetEditorColor() override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxDamageTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle DamageDataRow;
};
