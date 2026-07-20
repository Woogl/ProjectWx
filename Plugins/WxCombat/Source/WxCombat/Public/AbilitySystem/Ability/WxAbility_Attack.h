// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Attack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 공격 어빌리티.
 *
 * 사용 흐름:
 *  1. 약/강공격 입력 → ActivateAbility → 첫 콤보 몽타주 재생
 *  2. ANS_ComboWindow 구간 입력 → 경로에 L/H 추가 → 엔진 재발동으로 다음 몽타주 재생
 *  3. 터미널 노드(다음 경로 없음)의 ANS_ComboWindow 구간 입력 → 첫타로 재시작
 *  4. 콤보 미입력 → 몽타주 완료/중단 시 EndAbility
 *
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)으로 처리한다. 진행 신호는 곧 평범한 TryActivateAbility 재호출이며,
 * 콤보 윈도우 밖이거나 현재 노드를 잇지 못하는 입력은 CanActivateAbility가 재발동을 막아 현재 몽타주를 유지한다.
 * 콤보 단계마다 재발동되므로 비용/쿨다운(CommitAbility)과 OnActivateEffects가 단계마다 새로 적용된다.
 * 콤보가 끊김 없이 이어지려면 AbilityDataRow에서 단계 사이 간격보다 쿨다운을 짧게 잡거나 최대 충전 수를 단계 수 이상으로 둔다.
 *
 * ComboMap에 콤보 경로와 몽타주를 등록. (Key: L = 약공격, H = 강공격)
 * 타겟 방향 회전은 ANS_SnapToTarget이 담당.
 *
 * 예시 ComboMap 설정:
 *  "L"    → AM_Attack_L
 *  "H"    → AM_Attack_H
 *  "LL"   → AM_Attack_LL
 *  "LH"   → AM_Attack_LH
 *  "LLL"  → AM_Attack_LLL
 *  "LLH"  → AM_Attack_LLH
 *  "LHLH" → AM_Attack_LHLH
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Attack();

	/**
	 * 활성 중 재발동(콤보 진행)은 콤보 윈도우 안에서, 눌린 입력(L/H)이 현재 경로를 잇거나 터미널 재시작이 가능할 때만 허용한다.
	 * 이때 자기 차단(BlockAbilitiesWithTag=Ability)은 무시하되(직후 EndAbility가 해제) 사망/비용/쿨다운은 그대로 판정한다.
	 * 신규 발동은 엔진 순정 경로(Super)를 따른다.
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	/**
	 * 콤보 경로-몽타주 매핑.
	 * Key: 콤보 경로 (L = 약공격, H = 강공격. 예: "L", "LL", "LH", "LHLH")
	 * Value: 재생할 몽타주
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TMap<FName, TObjectPtr<UAnimMontage>> ComboMap;

private:
	/** 현재 경로의 몽타주를 재생한다 */
	void PlayComboMontage();

	/** 현재 경로에서 L 또는 H를 추가했을 때 유효한 분기가 있는지 반환 */
	bool HasNextCombo() const;

	/**
	 * 눌린 입력(LastPressedInputTag의 L/H)과 CurrentPath로 이번에 재생할 콤보 경로를 계산한다.
	 * 신규 발동이면 첫타(L/H), 콤보 진행이면 CurrentPath+입력(또는 터미널 시 첫타 재시작), 잇지 못하면 빈 문자열(무시).
	 * CanActivateAbility(게이트)와 ActivateAbility(적용)가 공유한다.
	 */
	FString ResolveNextComboPath() const;

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

	/**
	 * 현재 콤보 경로. 예: "L", "LL", "LLH". 비어 있으면 진행 중인 콤보가 없다(다음 발동은 첫타부터).
	 * 재발동 사이에는 보존되고, 콤보가 자연 종료되면 몽타주 핸들러가 비운다.
	 */
	FString CurrentPath;
};
