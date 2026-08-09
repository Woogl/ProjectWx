// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWxInputConfig;
class UWxLockOnManagerComponent;
class UInputAction;
struct FInputActionValue;

/**
 * 플레이어 캐릭터.
 * - SpringArm + Camera (3인칭 뷰)
 * - PossessedBy에서 ASC InitAbilityActorInfo 호출
 * - 게임플레이 입력(이동/시선/어빌리티) 소유. 입력 구성은 UWxInputConfig DA에서 주입.
 */
UCLASS()
class WXGAME_API AWxPlayerCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual bool CanCrouch() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Combat")
	TObjectPtr<UWxLockOnManagerComponent> LockOnManagerComponent;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch();

	void AbilityInputTriggered(const UInputAction* Action);
	void AbilityInputReleased(const UInputAction* Action);

	/** 캐릭터 입력 설정 (IMC + Move/Look + 어빌리티 바인딩) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxInputConfig> InputConfig;
};
