// Copyright Woogle. All Rights Reserved.

#include "Character/WxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Input/WxInputConfig.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "WxBGMSourceComponent.h"
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
	
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	LockOnManagerComponent = CreateDefaultSubobject<UWxLockOnManagerComponent>(TEXT("LockOnManagerComponent"));

	// 상호작용 목록 위젯.
	// Widget Class는 BP_Player에서 WBP_InteractionList로 지정한다.
	// 디폴트와 다른 값만 설정한다(DrawSize·Pivot은 UWidgetComponent 디폴트와 동일).
	InteractionListWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionListWidget"));
	InteractionListWidget->SetupAttachment(RootComponent);
	InteractionListWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionListWidget->SetDrawAtDesiredSize(true);
	InteractionListWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 원격 프록시 캐릭터에 중복 렌더되지 않도록 기본 숨김.
	// BeginPlay에서 로컬 컨트롤일 때만 표시한다.
	InteractionListWidget->SetVisibility(false);

	// 상태 기반 BGM 소스.
	// 실제 태그·우선순위는 BP_Player 에서 설정한다.
	BGMSourceComponent = CreateDefaultSubobject<UWxBGMSourceComponent>(TEXT("BGMSourceComponent"));

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void AWxPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Screen-space 위젯은 컴포넌트 월드 위치를 로컬 뷰포트에 투영하므로, 로컬 뷰어의 폰에서만 목록을 띄운다.
	if (IsLocallyControlled())
	{
		InteractionListWidget->SetVisibility(true);
	}
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

void AWxPlayerCharacter::ToggleCrouch()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
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
