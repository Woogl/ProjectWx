// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "WxComboMontageSelector.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;

/** 진입 조건으로 갈리는 콤보 몽타주 묶음. */
USTRUCT(BlueprintType)
struct FWxComboMontageSet
{
	GENERATED_BODY()

	/**
	 * 아바타 태그가 이 요구사항을 만족할 때 선택된다.
	 * 비우면 조건 없이 매칭(기본 콤보).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	FGameplayTagRequirements EntryTagRequirements;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;
};

/**
 * 아바타 태그로 콤보 세트를 하나 골라, 그 안에서 단계를 전진시키는 몽타주 선택기.
 *
 * MontageSets는 배열 순서가 곧 우선순위(첫 매칭 채택)이며, 진입 요구사항이 빈 세트는 조건 없이 매칭되므로 기본 콤보는 맨 아래에 둔다.
 * 세트는 콤보 진입 시 한 번 정해 끝까지 유지한다 — 도중에 태그가 사라져도 남은 단은 같은 세트로 이어진다.
 *
 * 진행 상태를 들고 있으므로 어빌리티가 인스턴스 멤버로 소유하며, 콤보를 끊을 때 Reset을 불러줘야 다음 발동이 세트부터 새로 고른다.
 */
USTRUCT(BlueprintType)
struct FWxComboMontageSelector
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TArray<FWxComboMontageSet> MontageSets;

	/**
	 * 다음 단으로 넘어가 그 몽타주를 돌려준다.
	 * 진행 중인 콤보가 없으면 아바타 태그로 세트부터 고르고, 터미널 단에서는 첫 단으로 되감긴다.
	 * 매칭되는 세트가 없거나 해당 단이 비어 있으면 nullptr.
	 */
	UAnimMontage* GetNextMontage(const UAbilitySystemComponent* ASC);

	void Reset();

private:
	int32 FindMontageSetIndex(const UAbilitySystemComponent* ASC) const;

	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다 */
	int32 CurrentSetIndex = INDEX_NONE;

	/** CurrentSetIndex와 함께 보존·리셋되는 ComboMontages 인덱스 */
	int32 CurrentMontageIndex = INDEX_NONE;
};
