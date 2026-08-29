// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Component/WxNameplateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxRewardLibrary.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Spawnable/WxSpawner.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxGameplayTags.h"

AWxEnemyCharacter::AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	NameplateComponent = CreateDefaultSubobject<UWxNameplateComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	NameplateComponent->InitializeViewModels(AbilitySystemComponent, CharacterName, Portrait);
}

bool AWxEnemyCharacter::IsInRearCone(const AActor* Interactor) const
{
	if (!Interactor)
	{
		return false;
	}

	FVector ToInteractor = Interactor->GetActorLocation() - GetActorLocation();
	ToInteractor.Z = 0.0;
	if (!ToInteractor.Normalize())
	{
		return false;
	}

	const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToInteractor);
	const float RearThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabRearHalfAngle));
	return ForwardDot <= RearThreshold;
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	if (!HasAuthority())
	{
		return;
	}

	if (AWxSpawner* Spawner = OwningSpawner.Get())
	{
		Spawner->MarkKilled();
	}

	// 처치자를 가리지 않고 항상 0번 플레이어에게 지급하는 것이 의도다.
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UWxRewardLibrary::GrantReward(this, RewardRow, PlayerController, GetActorTransform(), FVector::UpVector * LaunchSpeed);
	}
}

void AWxEnemyCharacter::OnInteracted(AActor* Interactor)
{
	// 서버 권위에서만 호출된다.
	if (!Interactor)
	{
		return;
	}

	// 앞잡·뒤잡 어빌리티가 같은 이벤트를 받고, 내 소유 태그(그로기 여부)에 대한 각자의 TargetTags 요건으로 하나만 성립한다.
	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = WxGameplayTags::Event_Finisher;
	AbilitySystemComponent->GetOwnedGameplayTags(EventData.TargetTags);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, WxGameplayTags::Event_Finisher, EventData);
}

FText AWxEnemyCharacter::GetInteractionPrompt() const
{
	return FText::FromString(TEXT("Finisher"));
}

AWxSpawner* AWxEnemyCharacter::GetOwningSpawner() const
{
	return OwningSpawner.Get();
}

void AWxEnemyCharacter::OnSpawnedBy(AWxSpawner* Spawner)
{
	OwningSpawner = Spawner;

	// 정찰 경로를 스포너에 그려 두므로 폰에서 거슬러 올라갈 링크가 필요하다(UWxPatrolComponent::FindPatrolComponent).
	// 부착을 고른 건 아웃라이너에서 어느 스포너 소속인지 그대로 보이기 때문이다.
	// 대가로 이동 복제가 ReplicatedMovement 가 아니라 AttachmentReplication 경로를 탄다 — 멀티 검증 때 재확인할 것.
	AttachToActor(Spawner, FAttachmentTransformRules::KeepWorldTransform);
}

bool AWxEnemyCharacter::CanInteract(const AActor* Interactor) const
{
	if (!IsAlive())
	{
		return false;
	}

	// 처형 당하기 어빌리티가 활성 구간(기상까지) 동안 발행한다.
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_BeingFinished))
	{
		return false;
	}

	// 그로기는 이미 무방비라 방향을 묻지 않는다(앞잡).
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		return true;
	}

	// 뒤잡은 적이 아직 나를 인지하지 못했을 때만 성립한다.
	return !AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat) && IsInRearCone(Interactor);
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

