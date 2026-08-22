// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Attack.generated.h"

class UAnimMontage;

/**
 * ComboMontages의 첫 몽타주를 재생하고, State.ComboWindow 구간의 재발동이 다음 단으로 넘긴다(터미널 단에서는 첫 단으로 되돌아간다).
 *
 * 콤보 진행은 엔진 순정 재발동(bRetriggerInstancedAbility)이라 단계마다 CommitAbility가 새로 걸린다.
 * 콤보가 끊기지 않으려면 AbilityDataRow에서 단계 간격보다 쿨다운을 짧게 잡거나 최대 충전 수를 단계 수 이상으로 둔다.
 *
 * 타겟 방향 회전은 WxAnimNotifyState_SnapToTarget이 담당.
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

	/** 콤보 미입력으로 끝났으므로 다음 발동은 첫 단부터 시작한다. */
	virtual void HandleMontageCompleted() override;

	/** 배열 순서가 곧 콤보 단계다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

private:
	/** 재발동 사이에 보존되며, INDEX_NONE이면 진행 중인 콤보가 없다. */
	int32 ComboIndex = INDEX_NONE;
};
