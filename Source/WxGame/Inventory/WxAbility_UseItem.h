// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "WxAbility_UseItem.generated.h"

/**
 * 소비 아이템 사용 어빌리티(다크소울 에스트병 방식).
 *
 * 쓸 소비 아이템은 인벤토리가 고른다. 지금 쓸 수 있는지(보유 + 충전 잔량)를 CanActivateAbility에서 검사해, 빈 병 입력이 발동해 앞 액션의 후딜을 취소하지 않게 한다.
 * 인벤토리와 인스턴스 충전량이 소유 클라에 복제되므로 이 판정은 클라에서도 성립한다.
 *
 * 충전 1 감소와 회복 GE 적용은 몽타주의 UseItem AnimNotify 시점에 ItemUseComponent가 처리한다.
 * 후딜 구간은 WxAnimNotify_StartRecovery 로 캔슬을 허용한다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_UseItem : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_UseItem();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
