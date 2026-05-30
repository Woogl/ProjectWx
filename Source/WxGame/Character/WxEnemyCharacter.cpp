// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Component/WxNameplateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "System/WxSpawnerSubsystem.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 회전은 베이스(AWxCharacterBase)의 설정을 그대로 사용한다.
	// bOrientRotationToMovement = true, RotationRate = 500°/s 로 이동 방향을 향해 부드럽게 회전한다.
	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// Full 모드: 모든 GE를 모든 클라이언트에 복제한다. 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
	// 적이 많아지거나 GE 수가 늘어날 경우 대역폭 비용을 측정하고, 필요하면 아래 방향으로 개선을 고려할 것:
	//   - UI 표시용 최소 데이터(Icon, Duration, StartWorldTime)만 담는 경량 복제 컴포넌트로 교체
	//   - UI에 표시할 이펙트에만 UWxEffectComponent_UIData를 붙이는 규칙을 엄수해 복제 대상 GE 수를 제한
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	NameplateComponent = CreateDefaultSubobject<UWxNameplateComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	NameplateComponent->InitializeViewModels(AbilitySystemComponent);
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	if (!HasAuthority())
	{
		return;
	}

	if (UWxSpawnerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWxSpawnerSubsystem>())
	{
		Subsystem->MarkSpawnableKilled(this);
	}
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

#if WITH_EDITOR
const UMeshComponent* AWxEnemyCharacter::GetEditorPreviewMeshComponent() const
{
	return GetMesh();
}
#endif

