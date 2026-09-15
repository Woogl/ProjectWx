// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxCharacterMovementComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 전 캐릭터 공용 CharacterMovementComponent — AWxCharacterBase 가 클래스를 교체해 파생 전부가 이걸 받는다.
 * 상승·하강에 서로 다른 중력 스케일을 적용해(비대칭 낙하) 액션성 있는 점프 감각을 낸다.
 *
 * 히트스톱 동안의 이동 정지는 캐릭터의 시간을 가진 머신만 건다.
 * 원격 클라 폰의 서버 사본은 ControlledCharacterMove 를 타지 않고, 클라가 무브를 보내지 않는 동안 저절로 멈춘다.
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
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	//~ End UCharacterMovementComponent Interface

protected:
	//~ Begin UCharacterMovementComponent Interface
	/** 이동 모드가 전 머신에 복제되므로 공중 태그 발행과 착지 처리를 각 머신이 스스로 한다. */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	/** 히트스톱 동안 무브를 만들지 않는다. 속도·넉업·이동 모드는 보존돼 풀리면 이어진다. */
	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds) override;

	/** 히트스톱 동안은 서버가 멈춰 복제 갱신이 끊기므로, 시뮬 프록시가 보존된 속도로 외삽하지 않게 한다. */
	virtual void SimulateMovement(float DeltaTime) override;
	//~ End UCharacterMovementComponent Interface

private:
	void JumpToLandingSection();

	bool IsHitStopped();

	/** 첫 호출에만 오너에서 해석해 기억해 둔다 — 이동 갱신마다 도는 탐색을 없앤다. */
	UAbilitySystemComponent* GetAbilitySystemComponent();

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
