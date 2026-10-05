// Copyright Woogle. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "WxAbility_Combo.generated.h"

class UAbilityTask_WaitInputPress;

/** 한 활성화 안에서 입력으로 몽타주 단계를 이어간다. 각 단계마다 비용·쿨다운을 커밋하고, 다음 단계를 커밋하지 못하면 입력만 버린다. */
UCLASS(Abstract, HideCategories = ("Wx|Montage"))
class WXGAME_API UWxAbility_Combo : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetMontage() const override;
	virtual float GetMontagePlayRate() const override;

	/**
	 * 몽타주 이벤트 태스크가 인스턴스 소유권을 검증한 뒤 호출한다.
	 * 창이 열린 동안만 다음 타 입력을 기다리므로, 창 밖 입력은 받는 태스크가 없어 ASC가 일반 발동으로 넘긴다.
	 */
	void OpenComboWindow();
	void CloseComboWindow();

	int32 GetComboIndex() const;

	/** 서버의 분신 실행. 진행 중이면 지정 단계로 넘기고, 비활성이면 정상 발동 조건부터 검사한다. */
	bool TryMirrorComboStep(int32 Index);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 배열 순서가 콤보 순서다. 한 단계도 배열에 하나를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Combo")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	/** INDEX_NONE이면 진행 중인 콤보가 없다. */
	int32 ComboIndex = INDEX_NONE;

private:
	bool PlayComboStep(int32 Index);

	UFUNCTION()
	void HandleComboInput(float TimeWaited);

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitInputPress> InputTask;

	int32 StartingComboIndex = 0;
};
