// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Interact.generated.h"

/**
 * 상호작용 어빌리티.
 *
 * 사용 흐름:
 *  1. 플레이어가 Input.Interact 입력 → ServerInitiated로 서버에서 활성화
 *  2. Avatar 캡슐의 오버랩 컴포넌트에서 UWxInteractionComponent 후보를 수집
 *  3. 가장 가까운 1개를 선택해 TryInteract 호출
 *  4. EndAbility (단발성)
 */
UCLASS()
class WXGAME_API UWxAbility_Interact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Interact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
