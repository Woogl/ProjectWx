// Copyright Woogle. All Rights Reserved.

#include "Character/WxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxInputBufferComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/WxInputConfig.h"
#include "Inventory/WxItemUseComponent.h"
#include "Finisher/WxFinisherDamageComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxCombatLibrary.h"
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

	LockOnComponent = CreateDefaultSubobject<UWxLockOnComponent>(TEXT("LockOnComponent"));
	FinisherDamageComponent = CreateDefaultSubobject<UWxFinisherDamageComponent>(TEXT("FinisherDamageComponent"));
	ItemUseComponent = CreateDefaultSubobject<UWxItemUseComponent>(TEXT("ItemUseComponent"));
	InputBufferComponent = CreateDefaultSubobject<UWxInputBufferComponent>(TEXT("InputBufferComponent"));

	StaminaWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("StaminaWidget"));
	StaminaWidget->SetupAttachment(RootComponent);
	StaminaWidget->SetWidgetSpace(EWidgetSpace::Screen);
	StaminaWidget->SetDrawAtDesiredSize(true);
	StaminaWidget->SetVisibility(false);

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void AWxPlayerCharacter::NotifyControllerChanged()
{
	// MappingContext 는 폰이 아니라 LocalPlayer 에 남으므로 빙의가 풀릴 때 직접 걷는다.
	// 떠난 컨트롤러는 Super 가 PreviousController 를 현재 값으로 갱신하기 전에만 읽을 수 있다.
	if (!IsLocallyControlled() && InputConfig && InputConfig->MappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystemFromController<UEnhancedInputLocalPlayerSubsystem>(Cast<APlayerController>(PreviousController)))
		{
			Subsystem->RemoveMappingContext(InputConfig->MappingContext);
		}
	}

	Super::NotifyControllerChanged();
	
	StaminaWidget->SetVisibility(IsLocallyControlled());
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
		EIC->BindAction(InputConfig->JumpAction, ETriggerEvent::Started,   this, &AWxPlayerCharacter::Jump);
		EIC->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (InputConfig->CrouchAction)
	{
		EIC->BindAction(InputConfig->CrouchAction, ETriggerEvent::Started, this, &AWxPlayerCharacter::ToggleCrouch);
	}

	for (const UInputAction* Action : AbilitySystemComponent->GetAbilityInputActions())
	{
		EIC->BindAction(Action, ETriggerEvent::Triggered, this, &AWxPlayerCharacter::AbilityInputTriggered, Action);
		EIC->BindAction(Action, ETriggerEvent::Completed, this, &AWxPlayerCharacter::AbilityInputReleased,  Action);
	}
}

void AWxPlayerCharacter::Jump()
{
	// 앉은 채로는 엔진이 점프를 막으므로 점프 입력을 기립 의사로 먼저 옮긴다.
	UnCrouch();

	Super::Jump();
}

void AWxPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 2단 점프는 스택되지 않는 GE를 하나 더 만들어, 늦게 걸린 쪽이 만료될 때까지 부여 태그가 유지된다.
	UWxCombatLibrary::ApplyEffect(AbilitySystemComponent, JumpInvincibleEffect, nullptr);
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
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_LockOn))
	{
		if (LockOnComponent)
		{
			LockOnComponent->SetLookInput(LookAxis);
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
	// 실패한 입력을 기억하는 선입력이 ASC 앞에 선다. 뗌은 기억할 것이 없어 ASC로 바로 간다.
	InputBufferComponent->InputActionTriggered(Action);
}

void AWxPlayerCharacter::AbilityInputReleased(const UInputAction* Action)
{
	AbilitySystemComponent->AbilityInputActionReleased(Action);
}
