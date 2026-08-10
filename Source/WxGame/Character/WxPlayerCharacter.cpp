// Copyright Woogle. All Rights Reserved.

#include "Character/WxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/WxInputConfig.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "WxGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxPlayerCharacter::AWxPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Player;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength      = 400.f;
	CameraBoom->TargetOffset         = FVector(0.f, 0.f, 35.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag     = true;
	CameraBoom->CameraLagSpeed       = 8.f;
	
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(60.f);

	JumpMaxCount = 2;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	LockOnManagerComponent = CreateDefaultSubobject<UWxLockOnManagerComponent>(TEXT("LockOnManagerComponent"));

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void AWxPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAbilitySystem();
}

void AWxPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!InputConfig)
	{
		return;
	}

	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (InputConfig->MappingContext)
		{
			Subsystem->AddMappingContext(InputConfig->MappingContext, 1);
		}
	}

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	if (InputConfig->MoveAction)
	{
		EIC->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::Move);
	}
	if (InputConfig->LookAction)
	{
		EIC->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::Look);
	}
	if (InputConfig->JumpAction)
	{
		EIC->BindAction(InputConfig->JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
		EIC->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (InputConfig->CrouchAction)
	{
		EIC->BindAction(InputConfig->CrouchAction, ETriggerEvent::Started, this, &AWxPlayerCharacter::ToggleCrouch);
	}
	// 어빌리티 입력 바인딩: 바인딩할 InputAction 목록은 AbilitySet의 부여 대상 어빌리티 CDO들에서 파생한다.
	//
	// Triggered: 트리거 조건을 만족한 시점. 같은 키에 걸린 Tap(회피)과 Hold(스프린트)를 가르므로 발동은 이쪽으로 받는다.
	// Completed: 트리거가 풀린 시점. 홀드형에서는 키를 뗀 순간이다.
	TArray<const UInputAction*> AbilityActions;
	if (AbilitySystemComponent)
	{
		AbilityActions = AbilitySystemComponent->GetAbilityInputActions();
	}
	for (const UInputAction* Action : AbilityActions)
	{
		EIC->BindAction(Action, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::AbilityInputTriggered, Action);
		EIC->BindAction(Action, ETriggerEvent::Completed, this, &AWxPlayerCharacter::AbilityInputReleased,  Action);
	}
}

bool AWxPlayerCharacter::CanCrouch() const
{
	const bool bParent = Super::CanCrouch();
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	return bParent && !bIsFalling;
}

void AWxPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator  YawRotation(0, Controller->GetControlRotation().Yaw, 0);

	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void AWxPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();

	// 락온 중에는 시점을 돌리지 않고 그 입력을 락온 컴포넌트에 넘겨 대상 전환에 쓰게 한다.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_LockOn))
	{
		if (LockOnManagerComponent)
		{
			LockOnManagerComponent->SetLookInput(LookAxis);
		}
		return;
	}

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AWxPlayerCharacter::ToggleCrouch()
{
	if (IsCrouched())
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AWxPlayerCharacter::AbilityInputTriggered(const UInputAction* Action)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputActionTriggered(Action);
	}
}

void AWxPlayerCharacter::AbilityInputReleased(const UInputAction* Action)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputActionReleased(Action);
	}
}
