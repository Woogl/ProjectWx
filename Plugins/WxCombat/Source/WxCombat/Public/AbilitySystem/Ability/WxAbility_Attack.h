// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Attack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
class UAnimMontage;

/**
 * 공격 어빌리티.
 *
 * 사용 흐름:
 *  1. 약/강공격 입력 → ActivateAbility → 첫 콤보 몽타주 재생
 *  2. ANS_ComboWindow 구간 입력 → 경로에 L/H 추가 → EndAbility 후 동일 spec 재발동, 다음 몽타주 재생
 *  3. 터미널 노드(다음 경로 없음)의 ANS_ComboWindow 구간 입력 → 첫타로 재시작 (EndAbility 후 재발동)
 *  4. 콤보 미입력 → 몽타주 완료/중단 시 EndAbility
 *
 * 콤보 단계마다 재발동되므로 비용/쿨다운(CommitAbility)과 OnActivateEffects가 단계마다 새로 적용된다.
 * 콤보가 끊김 없이 이어지려면 AbilityDataRow에서 단계 사이 간격보다 쿨다운을 짧게 잡거나 최대 충전 수를 단계 수 이상으로 둔다.
 *
 * ComboMap에 콤보 경로와 몽타주를 등록. (Key: L = 약공격, H = 강공격)
 * 타겟 방향 회전은 ANS_TurnAround이 담당.
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

	/** 콤보 입력 대기 태스크를 시작한다 */
	void WaitForComboInput();

	/** 현재 경로에서 L 또는 H를 추가했을 때 유효한 분기가 있는지 반환 */
	bool HasNextCombo() const;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleComboInputPressed(float TimeWaited);

	/** 현재 활성화를 종료하고 지정 경로로 동일 spec을 즉시 재발동한다 */
	void Reactivate(const FString& Path);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> WaitInputTask;

	/** 현재 콤보 경로. 예: "L", "LL", "LLH" */
	FString CurrentPath;

	/** 다음 ActivateAbility에서 사용할 콤보 경로. 비어있으면 신규 발동(입력으로 첫타 결정) */
	FString NextComboPath;
};
