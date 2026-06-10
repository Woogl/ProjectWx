// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "WxAbilitySet.generated.h"

class UWxAbilityBase;
class UGameplayEffect;
class UWxAbilitySystemComponent;

/**
 * AbilitySet이 부여할 단일 어빌리티 항목.
 * InputTag는 부여 시 Spec의 DynamicSpecSourceTags에 추가되어 입력 라우팅의 키가 된다.
 * 입력으로 발동하지 않는 어빌리티(AI 패턴 등)는 InputTag를 비워둔다.
 */
USTRUCT(BlueprintType)
struct FWxAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UWxAbilityBase> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/**
 * AbilitySet 부여 결과를 저장하는 핸들 구조체.
 * 부여된 Ability/Effect/AttributeSet을 나중에 일괄 제거할 때 사용.
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxAbilitySetGrantedHandles
{
	GENERATED_BODY()

	void RemoveFromAbilitySystem(UWxAbilitySystemComponent* ASC);

	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	TArray<FActiveGameplayEffectHandle> EffectHandles;
};

/**
 * Ability, Effect 초기 데이터를 하나의 에셋으로 관리하는 데이터 에셋.
 *
 * 캐릭터 BP에서 이 에셋을 지정하면 InitAbilityActorInfo 시점에
 * 포함된 모든 항목이 ASC에 일괄 부여된다.
 */
UCLASS(BlueprintType, Const)
class WXCOMBAT_API UWxAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ASC에 이 AbilitySet의 모든 항목을 부여한다. OutHandles에 결과를 저장 */
	void GiveToAbilitySystem(UWxAbilitySystemComponent* ASC, FWxAbilitySetGrantedHandles* OutHandles) const;

protected:
	/** 어트리뷰트 초기값 데이터테이블 Row 참조 (FWxCombatAttributeInitTableRow) */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes", meta = (RowType = "/Script/WxCombat.WxCombatAttributeInitTableRow"))
	FDataTableRowHandle AttributeInitRow;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<FWxAbilitySet_GameplayAbility> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
};
