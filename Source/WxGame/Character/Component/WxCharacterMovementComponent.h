// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxCharacterMovementComponent.generated.h"

class UAbilitySystemComponent;

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
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	//~ End UCharacterMovementComponent Interface

protected:
	//~ Begin UCharacterMovementComponent Interface
	/** 이동 모드가 전 머신에 복제되므로 공중 태그 발행과 착지 처리를 각 머신이 스스로 한다. */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	//~ End UCharacterMovementComponent Interface

private:
	void JumpToLandingSection();

	/** 첫 호출에만 오너에서 해석해 기억해 둔다 — 이동 갱신마다 도는 탐색을 없앤다. */
	UAbilitySystemComponent* GetAbilitySystemComponent();

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
