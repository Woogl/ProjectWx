// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "AI/WxEnemyController.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "View/MVVMView.h"
#include "WxViewModel_Health.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = true;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(GetRootComponent());
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawAtDesiredSize(true);
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UUserWidget* HealthBarWidget = HealthBarComponent->GetWidget())
	{
		if (UMVVMView* View = HealthBarWidget->GetExtension<UMVVMView>())
		{
			UWxViewModel_Health* ViewModel = NewObject<UWxViewModel_Health>(HealthBarWidget);
			ViewModel->InitializeWithASC(GetAbilitySystemComponent());
			View->SetViewModelByClass(ViewModel);
		}
	}
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	HealthBarComponent->SetVisibility(false);
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

