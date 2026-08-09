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

	/** 이 묶음의 진입 조건. 아바타 태그가 요구사항을 만족할 때 선택된다. 비우면 조건 없이 매칭(기본 콤보). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	FGameplayTagRequirements EntryTagRequirements;

	/** 인덱스 0부터 순서대로 재생할 콤보 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;
};

/**
 * 공격 어빌리티.
 *
 * 사용 흐름:
 *  1. 공격 입력 → ActivateAbility → 아바타 태그로 콤보 세트를 골라 첫 몽타주 재생
 *  2. ANS_ComboWindow 구간 재발동 → 같은 세트의 다음 인덱스 재생
 *  3. 터미널 인덱스에서 재발동 → 첫 인덱스로 재시작
 *  4. 콤보 미입력 → 몽타주 완료/중단 시 EndAbility
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
	 * 엔진이 차단을 캔슬보다 먼저 판정하므로, 뒤 경로가 없으면 취소 선언에 닿기도 전에 거부된다. 지목은 BP 데이터라 방향이 한쪽이다.
	 * 두 경로 모두 차단만 건너뛰고 사망·비용·쿨다운은 그대로 본다. 그 외 신규 발동은 Super를 따른다.
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

	/** 아바타 태그로 이번 콤보가 쓸 세트를 고른다. 매칭되는 세트가 없으면 INDEX_NONE */
	int32 ResolveMontageSetIndex() const;

	/** 현재 세트의 현재 인덱스 몽타주를 재생한다 */
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

	/** 진행 중인 콤보가 쓰는 MontageSets 인덱스. 재발동 사이에는 보존되고, INDEX_NONE이면 다음 발동에서 태그로 새로 고른다 */
	int32 CurrentSetIndex = INDEX_NONE;

	/** 현재 세트에서 재생 중인 ComboMontages 인덱스. CurrentSetIndex와 함께 보존·리셋된다 */
	int32 CurrentMontageIndex = INDEX_NONE;
};
