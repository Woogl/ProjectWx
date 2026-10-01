// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotifyState_WeaponAttack.generated.h"

/** 무기 타격 구간을 알린다. 어빌리티가 구간 시작 당시의 무기를 보관하고 판정을 정리한다. */
UCLASS()
class WXGAME_API UWxAnimNotifyState_WeaponAttack : public UWxAnimNotifyState_AbilityEvent
{
	GENERATED_BODY()

public:
	UWxAnimNotifyState_WeaponAttack();

#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

	
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxGame.WxDamageTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle DamageDataRow;
};
