// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "WxAbilityTask_WaitInputTagPressed.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxWaitInputTagPressedDelegate);

/**
 * 특정 입력 태그가 눌릴 때까지 대기하는 AbilityTask.
 *
 * UAbilityTask_WaitInputPress는 어빌리티에 라우팅되는 모든 입력에 반응하지만,
 * 이 태스크는 지정된 InputTag와 일치하는 입력만 감지한다.
 * UWxAbilitySystemComponent의 LastPressedInputTag를 기준으로 필터링한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_WaitInputTagPressed : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FWxWaitInputTagPressedDelegate OnPressed;

	static UWxAbilityTask_WaitInputTagPressed* CreateTask(UGameplayAbility* OwningAbility, FGameplayTag InInputTag);

	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnded) override;

private:
	FGameplayTag InputTag;
	FDelegateHandle DelegateHandle;

	void HandleInputPressed();
};
