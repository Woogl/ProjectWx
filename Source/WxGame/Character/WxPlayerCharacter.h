// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UGameplayEffect;
class UWxInputConfig;
class UWxItemUseComponent;
class UWxFinisherDamageComponent;
class UWxInputBufferComponent;
class UInputAction;
struct FInputActionValue;

/** 게임플레이 입력(이동/시선/어빌리티) 소유. 입력 구성은 UWxInputConfig DA에서 주입. */
UCLASS(Abstract)
class WXGAME_API AWxPlayerCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void NotifyControllerChanged() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;
	virtual void OnJumped_Implementation() override;
	virtual bool CanCrouch() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Combat")
	TObjectPtr<UWxFinisherDamageComponent> FinisherDamageComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Inventory")
	TObjectPtr<UWxItemUseComponent> ItemUseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxInputBufferComponent> InputBufferComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWidgetComponent> StaminaWidget;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch();

	void AbilityInputTriggered(const UInputAction* Action);
	void AbilityInputReleased(const UInputAction* Action);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxInputConfig> InputConfig;

	/** 도약 순간 자신에게 거는 무적. 지속시간은 GE가 쥐므로 HasDuration 이어야 한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Combat")
	TSubclassOf<UGameplayEffect> JumpInvincibleEffect;
};
