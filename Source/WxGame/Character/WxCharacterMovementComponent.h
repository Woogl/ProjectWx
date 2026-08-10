// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxCharacterMovementComponent.generated.h"

/**
 * 플레이어용 CharacterMovementComponent.
 * 상승·하강에 서로 다른 중력 스케일을 적용해(비대칭 낙하) 액션성 있는 점프 감각을 낸다.
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
	 * 어빌리티 몽타주가 재생 중인 동안에는 앉기 의사를 지운다.
	 * 앉은 채로 발동했다면 Super가 곧바로 일으켜 세우고, 재생 중 들어온 앉기 입력도 매 틱 여기서 취소된다.
	 * 락온처럼 몽타주를 쓰지 않는 어빌리티는 앉은 자세와 공존한다.
	 */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	//~ End UCharacterMovementComponent Interface
};
