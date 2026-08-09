// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "WxAbilitySet.generated.h"

class UWxAbilityBase;
class UGameplayEffect;
class UWxAbilitySystemComponent;
class UInputAction;

/** 부여 결과 핸들. 나중에 일괄 제거할 때 쓴다. */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxAbilitySetGrantedHandles
{
	GENERATED_BODY()

	void RemoveFromAbilitySystem(UWxAbilitySystemComponent* ASC);

	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	TArray<FActiveGameplayEffectHandle> EffectHandles;
};

/**
 * Ability, Effect 초기 데이터를 하나로 묶은 데이터 에셋.
 * 캐릭터 BP가 이 에셋을 지정하면 InitAbilityActorInfo 시점에 모든 항목이 ASC에 일괄 부여된다.
 */
UCLASS(BlueprintType, Const)
class WXCOMBAT_API UWxAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	void GiveToAbilitySystem(UWxAbilitySystemComponent* ASC, FWxAbilitySetGrantedHandles* OutHandles) const;

	/** 각 어빌리티 CDO가 요구하는 입력 액션 전체(중복 제거) */
	TArray<const UInputAction*> GetInputActions() const;

protected:
	/** 어트리뷰트 초기값 Row 참조 */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes", meta = (RowType = "/Script/WxCombat.WxCombatAttributeInitTableRow"))
	FDataTableRowHandle AttributeInitRow;

	/** 부여할 어빌리티 클래스 목록. 입력 라우팅 키는 각 어빌리티 CDO의 ActivationInputAction이 쥐고 있다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UWxAbilityBase>> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
};
