// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "WxAbilitySet.generated.h"

class UWxAbilityBase;
class UGameplayEffect;
class UWxAbilitySystemComponent;
class UInputAction;

/** 캐릭터 BP가 이 에셋을 지정하면 InitAbilitySystem 시점에 서버에서 모든 항목이 ASC에 일괄 부여된다. */
UCLASS(BlueprintType, Const)
class WXCOMBAT_API UWxAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	void GiveToAbilitySystem(UWxAbilitySystemComponent* ASC) const;

	/** 각 어빌리티 CDO가 요구하는 입력 액션 전체(중복 제거) */
	TArray<const UInputAction*> GetInputActions() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attributes", meta = (RowType = "/Script/WxCombat.WxCombatAttributeInitTableRow"))
	FDataTableRowHandle AttributeInitRow;

	/** 입력 라우팅 키는 각 어빌리티 CDO의 ActivationInputAction이 쥐고 있다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UWxAbilityBase>> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
};
