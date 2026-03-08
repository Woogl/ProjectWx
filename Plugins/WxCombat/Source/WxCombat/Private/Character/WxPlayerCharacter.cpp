// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/WxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Input/WxInputConfig.h"
#include "GAS/WxAbilitySystemComponent.h"

AWxPlayerCharacter::AWxPlayerCharacter()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength      = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
}

void AWxPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void AWxPlayerCharacter::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AWxPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::Move);
	}
	if (LookAction)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::Look);
	}

	// 어빌리티 입력 바인딩: InputConfig의 각 매핑에 대해 Press/Release 바인딩
	if (InputConfig)
	{
		for (const FWxInputAbilityBinding& Binding : InputConfig->AbilityInputBindings)
		{
			if (Binding.InputAction && Binding.InputTag.IsValid())
			{
				EIC->BindAction(Binding.InputAction, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::AbilityInputPressed,  Binding.InputTag);
				EIC->BindAction(Binding.InputAction, ETriggerEvent::Completed, this, &AWxPlayerCharacter::AbilityInputReleased, Binding.InputTag);
			}
		}
	}
}

void AWxPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator  YawRotation(0, Controller->GetControlRotation().Yaw, 0);

	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void AWxPlayerCharacter::Look(const FInputActionValue& Value)
{
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
