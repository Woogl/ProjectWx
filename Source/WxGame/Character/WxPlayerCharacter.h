// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UWxInputConfig;
class UWxLockOnManagerComponent;
class UInputAction;
struct FInputActionValue;

/**
 * 플레이어 캐릭터.
 * - 클라이언트에서는 OnRep_PlayerState 로 InitAbilitySystem 호출 (서버는 베이스의 PossessedBy)
 * - 게임플레이 입력(이동/시선/어빌리티) 소유. 입력 구성은 UWxInputConfig DA에서 주입.
 */
UCLASS()
class WXGAME_API AWxPlayerCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void NotifyControllerChanged() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual bool CanCrouch() const override;

protected:
	/** 스태미나 바 위젯에 어트리뷰트 뷰모델을 주입한다. */
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Combat")
	TObjectPtr<UWxLockOnManagerComponent> LockOnManagerComponent;

	/** 위젯 클래스는 BP 에서 지정한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWidgetComponent> StaminaWidget;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch();

	void AbilityInputTriggered(const UInputAction* Action);
	void AbilityInputReleased(const UInputAction* Action);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxInputConfig> InputConfig;
};
