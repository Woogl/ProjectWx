// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_UseItem.generated.h"

class UAnimMontage;
class UWxItemDefinition;
class UAbilityTask_WaitGameplayEvent;

/**
 * 소비 아이템 사용 어빌리티(다크소울 에스트병 방식).
 *
 * 발동은 입력(ActivationInputAction) 외에 인벤토리의 사용 요청으로도 일어난다.
 * 후자는 AssetTag(Ability.UseItem)로 이 어빌리티를 지목할 뿐 발동 자체는 입력과 같다.
 *
 * ConsumableDef 를 지금 쓸 수 있는지(보유 + 충전 잔량) 몽타주 전에 검사해 빈 병 모션을 막는다.
 * 인벤토리와 인스턴스 충전량이 소유 클라에 복제되므로 이 판정은 클라에서도 성립한다.
 *
	 * 충전 1 감소와 회복 GE 적용은 몽타주의 ExecuteAbilityAction(Event.AbilityAction.UseItem) 시점에 일어나며, 차감은 예측 대상이 아니라 이 단계만 서버 권위로 게이팅한다.
 * 후딜 구간은 WxAnimNotify_StartRecovery 로 캔슬을 허용한다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_UseItem : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_UseItem();

	/** 서버 권위로 ConsumableDef를 차감하고 사용 효과를 적용한다. */
	void UseConsumable();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 꿀꺽 지점에 UWxAnimNotify_ExecuteAbilityAction(Event.AbilityAction.UseItem)을 배치한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> UseMontage;

	/** Usable Fragment(회복 GE)와 Charges Fragment(충전)를 갖는 아이템이어야 한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UWxItemDefinition> ConsumableDef;

private:
	UFUNCTION()
	void HandleUseItemEvent(FGameplayEventData Payload);

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> UseItemEventTask;
};
