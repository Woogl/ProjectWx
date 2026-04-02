// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Attack.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

/**
 * 콤보 경로와 몽타주를 매핑하는 항목.
 */
USTRUCT(BlueprintType)
struct FWxComboEntry
{
	GENERATED_BODY()

	/**
	 * 콤보 경로 문자열. L = 약공격, H = 강공격.
	 * 예: "L", "LL", "LH", "LLL", "LLH", "LHLH"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString Path;

	/** 이 콤보 경로에서 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage;
};

/**
 * 공격 어빌리티.
 *
 * 사용 흐름:
 *  1. 약/강공격 입력 → ActivateAbility → 첫 콤보 몽타주 재생
 *  2. 콤보 윈도우 중 약/강공격 입력 → 경로에 L/H 추가 → 매칭되는 몽타주 재생
 *  3. 매칭되는 몽타주 없거나 콤보 미입력 → EndAbility
 *
 * ComboMap에 콤보 경로 문자열과 몽타주를 등록.
 * ANS_ComboWindow 구간에서 공격 입력 시 경로를 확장하여 다음 몽타주를 검색.
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
class WXCOMBAT_API UWxAbility_Attack : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 콤보 경로-몽타주 매핑 목록 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Combo")
	TArray<FWxComboEntry> ComboEntries;

private:
	/** 현재 경로의 몽타주를 재생한다 */
	void PlayComboMontage();

	/** 약공격/강공격 콤보 입력 대기 태스크를 시작한다 */
	void WaitForComboInput();

	/** 콤보 입력 대기 태스크를 정리한다 */
	void CleanupComboInputTasks();

	/** 현재 경로에서 L 또는 H를 추가했을 때 유효한 분기가 있는지 반환 */
	bool HasNextCombo() const;

	/** 경로에 해당하는 몽타주를 찾는다. 없으면 nullptr */
	UAnimMontage* FindMontage(const FString& InPath) const;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleComboLightInput(FGameplayEventData Payload);

	UFUNCTION()
	void HandleComboHeavyInput(FGameplayEventData Payload);

	/** 콤보 분기를 시도한다. 성공 시 true */
	bool TryAdvanceCombo(const FString& Suffix);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitLightInputTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHeavyInputTask;

	/** 현재 콤보 경로. 예: "L", "LL", "LLH" */
	FString CurrentPath;

	/** 런타임 룩업 맵. ComboEntries로부터 빌드 */
	UPROPERTY()
	TMap<FString, TObjectPtr<UAnimMontage>> ComboMap;
};
