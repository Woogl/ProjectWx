// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Effect.h"
#include "MVVM/WxViewModel_GameplayTag.h"

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
			UWxViewModel_Attribute* HealthViewModel = NewObject<UWxViewModel_Attribute>(AbilitySystemComponent);
			HealthViewModel->Initialize(AbilitySystemComponent, UWxCombatAttributeSet::GetHPAttribute(), UWxCombatAttributeSet::GetMaxHPAttribute());
			View->SetViewModel(TEXT("VM_Health"), HealthViewModel);

			UWxViewModel_Attribute* DazeViewModel = NewObject<UWxViewModel_Attribute>(AbilitySystemComponent);
			DazeViewModel->Initialize(AbilitySystemComponent, UWxCombatAttributeSet::GetDPAttribute(), UWxCombatAttributeSet::GetMaxDPAttribute());
			View->SetViewModel(TEXT("VM_Daze"), DazeViewModel);
			
			UWxViewModel_GameplayTag* GameplayTagViewModel = NewObject<UWxViewModel_GameplayTag>(AbilitySystemComponent);
			GameplayTagViewModel->Initialize(AbilitySystemComponent);
			View->SetViewModel(TEXT("VM_GameplayTag"), GameplayTagViewModel);
			
			UWxViewModel_AbilitySystem* AbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(AbilitySystemComponent);
			AbilitySystemViewModel->Initialize(AbilitySystemComponent);
			View->SetViewModel(TEXT("VM_AbilitySystem"), AbilitySystemViewModel);
		}
	}
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

