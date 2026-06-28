// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxPlayerCharacter.generated.h"

class UWxActivatableWidget;
class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UWxInputConfig;
class UWxLockOnManagerComponent;
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
	AWxPlayerCharacter();
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	TSubclassOf<UWxActivatableWidget> GetGameHUDClass() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Combat")
	TObjectPtr<UWxLockOnManagerComponent> LockOnManagerComponent;

	/** 상호작용 목록(WBP_InteractionList) 표시용 Screen-space 위젯 컴포넌트. Widget Class는 BP_Player에서 지정한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|UI")
	TObjectPtr<UWidgetComponent> InteractionListWidget;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch();

	void AbilityInputPressed(FGameplayTag InputTag);
	void AbilityInputReleased(FGameplayTag InputTag);

	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI")
	TSubclassOf<UWxActivatableWidget> GameHUDClass;

	/** 캐릭터 입력 설정 (IMC + Move/Look + 어빌리티 바인딩) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UWxInputConfig> InputConfig;
};
