// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility_Combo.h"
#include "WxAbility_Skill.generated.h"

/**
 * ComboMontages의 첫 몽타주를 재생하고, 콤보 창 구간의 재발동이 다음 단으로 넘긴다(터미널 단에서는 첫 단으로 되돌아간다).
 *
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)이다.
 * 진행 신호가 평범한 TryActivateAbility 재호출이라 하드웨어 입력과 UI 버튼이 같은 경로를 쓰고, 단계마다 CommitAbility가 새로 걸린다.
 *
 * 단계가 하나면 ComboWindow를 배치하지 않는 한 그 하나만 재생하고 종료한다.
 * 타겟 방향 회전은 WxAnimNotifyState_SnapToTarget이 담당.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Skill : public UWxAbility_Combo
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 몽타주 재생이 완료되면 다음 발동은 첫 단부터 시작한다. */
	virtual void HandleMontageCompleted() override;

	virtual void OnComboWindowClosed() override;
};
