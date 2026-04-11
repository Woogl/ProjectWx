// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxPlayerCharacter.generated.h"

class UWxActivatableWidget;
class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class UWxInteractionComponent;
struct FInputActionValue;

/**
 * 플레이어 캐릭터.
 * - SpringArm + Camera (3인칭 뷰)
 * - PossessedBy에서 ASC InitAbilityActorInfo 호출
 * - 입력 데이터는 WxPlayerController에서 관리
 */
UCLASS()
class WXGAME_API AWxPlayerCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	TSubclassOf<UWxActivatableWidget> GetGameHUDClass() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	virtual void OnRep_PlayerState() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void AbilityInputPressed(FGameplayTag InputTag);
	void AbilityInputReleased(FGameplayTag InputTag);

	UFUNCTION()
	void HandleInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI")
	TSubclassOf<UWxActivatableWidget> GameHUDClass;
};
