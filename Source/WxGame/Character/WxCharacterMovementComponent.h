// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxCharacterMovementComponent.generated.h"

/**
 * 전 캐릭터 공용 CharacterMovementComponent — AWxCharacterBase 가 클래스를 교체해 파생 전부가 이걸 받는다.
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

protected:
	//~ Begin UCharacterMovementComponent Interface
	/** 낙하 모드 진입·이탈을 공중 상태 태그로 발행하고, 착지 시 공중 몽타주를 마무리 섹션으로 넘긴다. 이동 모드가 전 머신에 복제되므로 각 머신이 제 몫을 스스로 처리한다. */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	//~ End UCharacterMovementComponent Interface

private:
	/** 재생 중인 몽타주가 착지 섹션을 가지고 있을 때만 그리로 넘긴다 — 섹션의 존재가 곧 착지 반응 여부다. */
	void JumpToLandingSection();
};
