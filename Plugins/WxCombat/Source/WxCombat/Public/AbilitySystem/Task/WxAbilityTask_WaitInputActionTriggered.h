// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_WaitInputActionTriggered.generated.h"

class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxWaitInputActionTriggeredDelegate);

/**
 * 특정 입력 액션이 트리거될 때까지 대기하는 AbilityTask.
 *
 * UAbilityTask_WaitInputPress는 어빌리티에 라우팅되는 모든 입력에 반응하지만, 이 태스크는 지정된 InputAction과 일치하는 입력만 감지한다.
 * UWxAbilitySystemComponent의 OnInputActionTriggered 방송을 구독해 액션으로 필터링한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_WaitInputActionTriggered : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FWxWaitInputActionTriggeredDelegate OnTriggered;

	static UWxAbilityTask_WaitInputActionTriggered* CreateTask(UGameplayAbility* OwningAbility, const UInputAction* InInputAction);

	virtual void OnDestroy(bool AbilityEnded) override;

protected:
	virtual void Activate() override;

private:
	UPROPERTY()
	TObjectPtr<const UInputAction> InputAction;

	FDelegateHandle DelegateHandle;

	void HandleInputTriggered(const UInputAction* Action);
};
