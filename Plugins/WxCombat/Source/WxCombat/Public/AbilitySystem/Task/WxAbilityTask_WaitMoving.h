// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_WaitMoving.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnMovingChanged, bool, bIsMoving);

/**
 * 아바타의 수평 이동 여부를 폴링해 상태가 바뀐 프레임에만 통지한다.
 * 이동 시작·정지에는 엔진 이벤트가 없어 폴링이 유일한 감지 수단이다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_WaitMoving : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_WaitMoving* CreateTask(UGameplayAbility* OwningAbility, float InMinSpeed = 10.f);

	/** 활성화 직후 현재 상태로 한 번, 이후 전환마다 브로드캐스트한다 */
	UPROPERTY()
	FWxOnMovingChanged OnMovingChanged;

	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void Activate() override;

private:
	bool IsAvatarMoving() const;

	float MinSpeedSquared = 10.f * 10.f;

	bool bIsMoving = false;
};
