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

	// Full 모드: 모든 GE를 모든 클라이언트에 복제한다. 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
	// 적이 많아지거나 GE 수가 늘어날 경우 대역폭 비용을 측정하고, 필요하면 아래 방향으로 개선을 고려할 것:
	//   - UI 표시용 최소 데이터(Icon, Duration, StartWorldTime)만 담는 경량 복제 컴포넌트로 교체
	//   - UI에 표시할 이펙트에만 UWxEffectComponent_UIData를 붙이는 규칙을 엄수해 복제 대상 GE 수를 제한
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

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

