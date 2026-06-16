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
 *  2. 로컬 플레이어의 레지스트리(UWxInteractionRegistrySubsystem)에서 현재 선택된 컴포넌트를 읽음
 *  3. 선택된 컴포넌트가 있으면 TryInteract 호출(없으면 아무 동작 없음)
 *  4. EndAbility (단발성)
 *
 * 선택은 LocalPlayerSubsystem이 로컬로 소유하므로 GetLocalPlayer가 유효한 스탠드얼론/리슨서버 호스트 기준이다.
 */
UCLASS()
class WXGAME_API UWxAbility_Interact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Interact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
