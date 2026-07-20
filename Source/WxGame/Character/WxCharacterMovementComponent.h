// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxCharacterMovementComponent.generated.h"

/**
 * 플레이어용 CharacterMovementComponent.
 * 상승·하강에 서로 다른 중력 스케일을 적용해(비대칭 낙하) 액션성 있는 점프 감각을 낸다.
 * 정점에서 덜 머물고 더 빠르게 떨어진다.
 */
UCLASS()
class WXGAME_API UWxCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UWxCharacterMovementComponent();

	//~ Begin UMovementComponent Interface
	virtual float GetGravityZ() const override;
	//~ End UMovementComponent Interface

protected:
	/** 상승 중(Velocity.Z >= 0) 중력 스케일. 값이 작을수록 천천히 올라간다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Jump", meta = (ClampMin = "0.0"))
	float RiseGravityScale = 2.f;

	/** 하강 중(Velocity.Z < 0) 중력 스케일. RiseGravityScale보다 크게 두어 빠른 낙하감을 만든다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Jump", meta = (ClampMin = "0.0"))
	float FallGravityScale = 2.5f;
};
