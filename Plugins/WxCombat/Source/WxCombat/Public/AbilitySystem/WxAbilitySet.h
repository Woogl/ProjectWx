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

/** 캐릭터 BP가 ASC의 AbilitySets에 이 에셋을 넣으면 InitAbilitySystem 시점에 서버에서 모든 항목이 ASC에 일괄 부여된다. */
UCLASS(BlueprintType, Const)
class WXCOMBAT_API UWxAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 캐릭터가 받는 세트 전체에서 같은 어빌리티는 한 번만 부여한다. */
	void GiveToAbilitySystem(UWxAbilitySystemComponent* ASC) const;

	/** 없는 어빌리티만 부여한다. 재등록으로 스펙이 사라져도 속성과 GE는 다시 적용하지 않는다. */
	void GiveAbilitiesToAbilitySystem(UWxAbilitySystemComponent* ASC) const;

	/** 여러 세트를 한 배열에 모으므로 중복 제거는 받은 배열 기준이다. */
	void AppendInputActions(TArray<const UInputAction*>& OutInputActions) const;

#if WITH_EDITOR
	/**
	 * 어빌리티 사이의 규칙을 이 세트 안에서만 본다. 어빌리티 하나의 규칙은 GA_가 본다.
	 * 풀리지 않는 속성 행은 오류다.
	 * 빈 칸, 같은 어빌리티 중복, 같은 입력의 어빌리티끼리 겹치는 조건, 같은 쿨다운 태그의 다른 값은 경고다.
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attributes", meta = (RowType = "/Script/WxCombat.WxCombatAttributeInitTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle AttributeInitRow;

	/** 입력 라우팅 키는 각 어빌리티 CDO의 ActivationInputAction이 쥐고 있다. 같은 입력의 어빌리티가 여럿이면 이 순서로 시도해 처음 성공한 것을 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UWxAbilityBase>> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
};
