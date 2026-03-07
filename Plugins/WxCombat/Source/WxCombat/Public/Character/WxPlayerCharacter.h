// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 플레이어 캐릭터.
 * - SpringArm + Camera (3인칭 뷰)
 * - Enhanced Input 이동/시점
 * - PossessedBy에서 ASC InitAbilityActorInfo 호출
 */
UCLASS()
class WXCOMBAT_API AWxPlayerCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ── Input ──────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> JumpAction;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	virtual void InitAbilityActorInfo() override;
};
