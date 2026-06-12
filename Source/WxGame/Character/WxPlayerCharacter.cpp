// Copyright Woogle. All Rights Reserved.

#include "Character/WxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Input/WxInputConfig.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "WxGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxPlayerCharacter::AWxPlayerCharacter()
{
	Team = EWxTeam::Player;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength      = 400.f;
	CameraBoom->TargetOffset         = FVector(0.f, 0.f, 35.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag     = true;
	CameraBoom->CameraLagSpeed       = 8.f;
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

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

	// 어빌리티 입력 바인딩: 각 매핑에 대해 Press/Release 바인딩
	for (const FWxInputAbilityBinding& Binding : InputConfig->AbilityInputBindings)
	{
		if (Binding.InputAction && Binding.InputTag.IsValid())
		{
			EIC->BindAction(Binding.InputAction, ETriggerEvent::Started,   this, &AWxPlayerCharacter::AbilityInputPressed,  Binding.InputTag);
			EIC->BindAction(Binding.InputAction, ETriggerEvent::Completed, this, &AWxPlayerCharacter::AbilityInputReleased, Binding.InputTag);
		}
	}
}

TSubclassOf<UWxActivatableWidget> AWxPlayerCharacter::GetGameHUDClass() const
{
	return GameHUDClass;
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
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_LockOn))
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AWxPlayerCharacter::AbilityInputPressed(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void AWxPlayerCharacter::AbilityInputReleased(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}
