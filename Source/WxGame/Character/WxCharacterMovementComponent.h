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

	//~ Begin UCharacterMovementComponent Interface
	/**
	 * 락온을 제외한 어빌리티를 수행하는 동안에는 앉기 의사를 지운다.
	 * 앉은 채로 발동했다면 Super가 곧바로 일으켜 세우고, 수행 중 들어온 앉기 입력도 매 틱 여기서 취소된다.
	 */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	//~ End UCharacterMovementComponent Interface
};
