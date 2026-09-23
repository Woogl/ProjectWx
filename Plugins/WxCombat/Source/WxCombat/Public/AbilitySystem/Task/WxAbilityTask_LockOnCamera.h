// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_LockOnCamera.generated.h"

class UWxLockOnComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnRetargetRequested, FVector2D, ScreenDirection);

/**
 * 아바타 락온 컴포넌트의 대상을 매 틱 읽어 컨트롤러 회전을 그쪽으로 보간한다.
 * 대상이 없어지거나(파괴 포함) 락온할 수 없게 되거나 거리를 벗어나면 OnTargetLost를 쏜다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_LockOnCamera : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_LockOnCamera* CreateTask(UGameplayAbility* OwningAbility, float InInterpSpeed = 10.f, float InPitchOffset = -15.f, float InMaxDistance = 2000.f, float InRetargetLookThreshold = 40.f);

	UPROPERTY()
	FWxOnTargetLost OnTargetLost;

	/** 시선 입력을 임계값 이상 누적했을 때 정규화된 화면 기준 방향을 전달하며 재탐색을 요청한다. */
	UPROPERTY()
	FWxOnRetargetRequested OnRetargetRequested;

	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void Activate() override;

private:
	TWeakObjectPtr<UWxLockOnComponent> LockOnComponent;
	float InterpSpeed = 8.f;
	float PitchOffset = -15.f;
	float MaxDistanceSquared = 2000.f * 2000.f;
	float RetargetLookThreshold = 40.f;
	FVector2D AccumulatedLook = FVector2D::ZeroVector;
};
