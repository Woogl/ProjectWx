// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/EngineTypes.h"
#include "WxAbility_Groggy.generated.h"

class UAnimMontage;
struct FOnAttributeChangeData;

/**
 * GP가 MaxGP에 닿을 때 AttributeSet이 송출하는 Event.Groggy로 발동하고, 폴링으로 빈 몽타주 슬롯에 그로기 몽타주를 밀어 넣으며 GP를 드레인한다.
 * GP가 0이 되면 스스로 종료한다 — 활성 동안 부여되는 Ability.Groggy가 곧 그로기 상태다.
 *
 * 드레인과 종료 판정은 서버만 한다. 소유 클라 인스턴스는 자세 몽타주만 맡고, 종료는 서버가 보내는 ClientEndAbility로 따라간다.
 */
// TODO: 단순하게 몽타주 길이로만 판정하자
UCLASS()
class WXCOMBAT_API UWxAbility_Groggy : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Groggy();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UAnimMontage> GroggyMontage;

private:
	void HandleGPChanged(const FOnAttributeChangeData& Data);

	void HandleMontagePollTick();

	FDelegateHandle GPDelegateHandle;
	FActiveGameplayEffectHandle DrainGPEffectHandle;
	FTimerHandle MontagePollingTimerHandle;
};
