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
class WXGAME_API UWxAbilityTask_LockOnCamera : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_LockOnCamera* CreateTask(UGameplayAbility* OwningAbility, float InInterpSpeed, float InPitchOffset, float InMaxDistance, float InRetargetLookThreshold);

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
	float InterpSpeed;
	float PitchOffset;
	float MaxDistanceSquared;
	float RetargetLookThreshold;
	FVector2D AccumulatedLook = FVector2D::ZeroVector;
};
