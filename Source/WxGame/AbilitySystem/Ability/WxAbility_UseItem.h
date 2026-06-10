// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_UseItem.generated.h"

class UAnimMontage;
class UWxItemDefinition;

/**
 * 소비 아이템 사용 어빌리티(다크소울 에스트병 방식).
 *
 * 사용 흐름:
 *  1. 입력 → ServerInitiated 로 서버에서 활성화 (인벤토리 차감은 서버 권한 작업)
 *  2. ConsumableDef 를 지금 사용할 수 있는지(보유 + 충전 잔량) 검사 — 불가하면 즉시 종료(빈 병 모션 방지)
 *  3. UseMontage(마시기 모션) 재생
 *  4. 몽타주의 WxAnimNotify_UseItem(Event.UseItem) 시점에 UseItemByDef 호출 → 충전 1 감소 + 회복 GE 적용
 *  5. 후딜 구간은 WxAnimNotify_StartRecovery 로 캔슬 허용, 몽타주 완료/중단 시 EndAbility
 *
 * 사용할 아이템과 회복 효과는 ConsumableDef(ItemDefinition)의 Usable/Charges Fragment 가 정의한다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_UseItem : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_UseItem();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 마시기(사용) 몽타주. 꿀꺽 지점에 WxAnimNotify_UseItem 노티파이를 배치한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> UseMontage;

	/** 사용할 소비 아이템 정의. Usable Fragment(회복 GE)와 Charges Fragment(충전)를 갖는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UWxItemDefinition> ConsumableDef;

private:
	UFUNCTION()
	void HandleConsumeEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
