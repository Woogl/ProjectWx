// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "AI/WxEnemyController.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = true;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	NameplateComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	NameplateComponent->SetWidgetSpace(EWidgetSpace::Screen);
	NameplateComponent->SetDrawAtDesiredSize(true);
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UUserWidget* NameplateWidget = NameplateComponent->GetWidget())
	{
		if (UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>())
		{
			UWxViewModel_Attribute* HealthViewModel = NewObject<UWxViewModel_Attribute>(NameplateWidget);
			HealthViewModel->Initialize(GetAbilitySystemComponent(), UWxCombatAttributeSet::GetHPAttribute(), UWxCombatAttributeSet::GetMaxHPAttribute());
			View->SetViewModel(TEXT("Health"), HealthViewModel);

			UWxViewModel_Attribute* DazeViewModel = NewObject<UWxViewModel_Attribute>(NameplateWidget);
			DazeViewModel->Initialize(GetAbilitySystemComponent(), UWxCombatAttributeSet::GetDPAttribute(), UWxCombatAttributeSet::GetMaxDPAttribute());
			View->SetViewModel(TEXT("Daze"), DazeViewModel);
		}
	}
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

