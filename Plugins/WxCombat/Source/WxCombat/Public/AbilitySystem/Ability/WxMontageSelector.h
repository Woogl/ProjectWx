// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "WxMontageSelector.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;

/** 진입 조건으로 갈리는 몽타주 묶음. 여럿이면 순서가 곧 콤보 단계다. */
USTRUCT(BlueprintType)
struct FWxMontageSet
{
	GENERATED_BODY()

	/**
	 * 아바타 태그가 이 요구사항을 만족할 때 후보가 된다.
	 * 비우면 조건 없이 매칭.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Montage")
	FGameplayTagRequirements AvatarTagRequirements;

	/**
	 * 어빌리티를 깨운 이벤트 태그가 여기 걸릴 때 후보가 된다. 부모 태그 하나로 자식 전부를 받는다.
	 * 비우면 이벤트를 보지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Montage")
	FGameplayTagContainer TriggerEventTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Montage")
	TArray<TObjectPtr<UAnimMontage>> Montages;
};

/**
 * 아바타 태그와 트리거 이벤트 태그로 몽타주 세트를 하나 골라, 그 안에서 재생할 몽타주를 답한다.
 *
 * MontageSets는 배열 순서가 곧 우선순위이며, 두 조건을 모두 만족하는 첫 세트를 채택한다.
 * 조건을 비운 세트는 무조건 매칭되므로 배열 마지막에 두면 그것이 폴백이다.
 *
 * 진행 상태를 들고 있으므로 어빌리티가 인스턴스 멤버로 소유한다.
 */
USTRUCT(BlueprintType)
struct FWxMontageSelector
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Montage")
	TArray<FWxMontageSet> MontageSets;

	/**
	 * 세트를 새로 골라 그 첫 몽타주를 돌려준다.
	 * 진행 중이던 상태는 버려지므로 이전 발동이 무엇이었든 결과가 같다.
	 */
	UAnimMontage* SelectMontage(const UAbilitySystemComponent* ASC, FGameplayTag TriggerEventTag = FGameplayTag());

	/**
	 * 다음 단으로 넘어가 그 몽타주를 돌려준다.
	 * 진행 중인 콤보가 없으면 세트부터 고르고, 터미널 단에서는 첫 단으로 되감긴다.
	 * 매칭되는 세트가 없거나 해당 단이 비어 있으면 nullptr.
	 */
	UAnimMontage* AdvanceMontage(const UAbilitySystemComponent* ASC, FGameplayTag TriggerEventTag = FGameplayTag());

	void Reset();

private:
	int32 FindMontageSetIndex(const UAbilitySystemComponent* ASC, FGameplayTag TriggerEventTag) const;

	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다 */
	int32 CurrentSetIndex = INDEX_NONE;

	int32 CurrentMontageIndex = INDEX_NONE;
};
