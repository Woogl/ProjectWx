// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "WxAbilityTask_WaitInputTagReleased.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxWaitInputTagReleasedDelegate);

/**
 * 특정 입력 태그가 해제될 때까지 대기하는 AbilityTask.
 *
 * UAbilityTask_WaitInputRelease는 어빌리티에 라우팅되는 모든 입력에 반응하지만,
 * 이 태스크는 지정된 InputTag와 일치하는 입력만 감지한다.
 * UWxAbilitySystemComponent의 LastReleasedInputTag를 기준으로 필터링한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_WaitInputTagReleased : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FWxWaitInputTagReleasedDelegate OnReleased;

	static UWxAbilityTask_WaitInputTagReleased* CreateTask(UGameplayAbility* OwningAbility, FGameplayTag InInputTag);

	virtual void OnDestroy(bool AbilityEnded) override;

protected:
	virtual void Activate() override;

private:
	FGameplayTag InputTag;
	FDelegateHandle DelegateHandle;

	void HandleInputReleased();
};
