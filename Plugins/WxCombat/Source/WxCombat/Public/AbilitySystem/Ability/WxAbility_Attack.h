// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Attack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/** 진입 조건으로 갈리는 콤보 몽타주 묶음. */
USTRUCT(BlueprintType)
struct FWxAbilityMontageSet
{
	GENERATED_BODY()

	/** 아바타 태그가 이 요구사항을 만족할 때 선택된다. 비우면 조건 없이 매칭(기본 콤보). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	FGameplayTagRequirements EntryTagRequirements;

	/** 인덱스 0부터 순서대로 재생할 콤보 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;
};

/**
 * 공격 어빌리티.
 * 아바타 태그로 콤보 세트를 골라 첫 몽타주를 재생하고, ANS_ComboWindow 구간의 재발동이 같은 세트의 다음 단으로 넘긴다(터미널 단에서는 첫 단으로 되돌아간다).
 *
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)이라 단계마다 CommitAbility가 새로 걸린다.
 * 콤보가 끊기지 않으려면 AbilityDataRow에서 단계 간격보다 쿨다운을 짧게 잡거나 최대 충전 수를 단계 수 이상으로 둔다.
 *
 * MontageSets는 배열 순서가 곧 우선순위(첫 매칭 채택)이며, 진입 요구사항이 빈 세트는 조건 없이 매칭되므로 기본 콤보는 맨 아래에 둔다.
 * 세트는 콤보 진입 시 한 번 정해 끝까지 유지한다 — 도중에 태그가 사라져도 남은 단은 같은 세트로 이어진다.
 * 타겟 방향 회전은 ANS_SnapToTarget이 담당.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Attack();

	/**
	 * 차단을 넘어 발동하는 두 경로를 연다 — 콤보 윈도우 안의 재발동, 그리고 CancelAbilitiesWithTag로 지목한 어빌리티가 돌고 있을 때의 진입.
	 * 엔진이 차단을 캔슬보다 먼저 판정하므로 뒤 경로가 없으면 취소 선언에 닿기도 전에 거부된다. 지목은 BP 데이터라 방향이 한쪽이다.
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 상태별 콤보 세트 목록. 위에서부터 훑어 진입 요구사항을 만족하는 첫 세트를 쓴다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TArray<FWxAbilityMontageSet> MontageSets;

private:
	/** CancelAbilitiesWithTag로 지목한 어빌리티가 이 ASC에서 하나라도 활성인지 */
	bool HasActiveCancelTarget(const UAbilitySystemComponent& ASC) const;

	int32 ResolveMontageSetIndex() const;

	void PlayCurrentMontage();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	/** 재발동 사이에 보존되며, INDEX_NONE이면 다음 발동에서 태그로 새로 고른다 */
	int32 CurrentSetIndex = INDEX_NONE;

	/** CurrentSetIndex와 함께 보존·리셋되는 ComboMontages 인덱스 */
	int32 CurrentMontageIndex = INDEX_NONE;
};
