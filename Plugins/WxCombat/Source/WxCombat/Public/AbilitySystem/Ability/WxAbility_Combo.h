// Copyright Woogle. All Rights Reserved.

#pragma once

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Combo.generated.h"

/** 단계는 몽타주 배열로, 각 단계의 방향은 몽타주 섹션으로 구성한다. */
UCLASS(Abstract, HideCategories = ("Wx|Montage"))
class WXCOMBAT_API UWxAbility_Combo : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetMontage() const override;

protected:
	/** 배열 순서가 콤보 순서다. 한 단계도 배열에 하나를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다. */
	int32 ComboIndex = INDEX_NONE;
};
