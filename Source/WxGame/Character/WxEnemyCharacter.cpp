// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffectComponent_UIData.h"
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
			UWxViewModel_Attribute* HealthViewModel = NewObject<UWxViewModel_Attribute>(NameplateWidget);
			HealthViewModel->Initialize(GetAbilitySystemComponent(), UWxCombatAttributeSet::GetHPAttribute(), UWxCombatAttributeSet::GetMaxHPAttribute());
			View->SetViewModel(TEXT("Health"), HealthViewModel);

			UWxViewModel_Attribute* DazeViewModel = NewObject<UWxViewModel_Attribute>(NameplateWidget);
			DazeViewModel->Initialize(GetAbilitySystemComponent(), UWxCombatAttributeSet::GetDPAttribute(), UWxCombatAttributeSet::GetMaxDPAttribute());
			View->SetViewModel(TEXT("Daze"), DazeViewModel);
			
			UWxViewModel_GameplayTag* GameplayTagViewModel = NewObject<UWxViewModel_GameplayTag>(NameplateWidget);
			GameplayTagViewModel->Initialize(GetAbilitySystemComponent());
			View->SetViewModel(TEXT("GameplayTag"), GameplayTagViewModel);
		}
	}
	
	AbilitySystemComponent->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &AWxEnemyCharacter::HandleEffectApplied);
	AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &AWxEnemyCharacter::HandleEffectRemoved);
}

void AWxEnemyCharacter::HandleEffectApplied(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle ActiveEffect)
{
	if (UUserWidget* NameplateWidget = NameplateComponent->GetWidget())
	{
		if (UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>())
		{
			if (const UWxEffectComponent_UIData* UIData = Spec.Def->FindComponent<UWxEffectComponent_UIData>())
			{
				UWxViewModel_Effect* EffectViewModel = NewObject<UWxViewModel_Effect>(NameplateWidget);
				FText EffectName = UIData->DisplayName;
				UTexture2D* EffectIcon = UIData->Icon.IsNull() ? nullptr : UIData->Icon.LoadSynchronous();
				EffectViewModel->Initialize(ASC, ActiveEffect, EffectName, EffectIcon);
				View->SetViewModel(TEXT("Effect"), EffectViewModel);
			}
		}
	}
}

void AWxEnemyCharacter::HandleEffectRemoved(const FActiveGameplayEffect& ActiveEffect)
{
	if (UUserWidget* NameplateWidget = NameplateComponent->GetWidget())
	{
		if (UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>())
		{
			if (TScriptInterface<INotifyFieldValueChanged> ViewModelInterface = View->GetViewModel(TEXT("Effect")))
			{
				if (UWxViewModel_Effect* EffectViewModel = Cast<UWxViewModel_Effect>(ViewModelInterface.GetObject()))
				{
					if (EffectViewModel->GetBoundHandle() == ActiveEffect.Handle)
					{
						View->SetViewModel(TEXT("Effect"), nullptr);
					}
				}
			}
		}
	}
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

