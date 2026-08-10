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

AWxEnemyCharacter::AWxEnemyCharacter()
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// Full 모드: 모든 GE를 모든 클라이언트에 복제한다.
	// 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
	// 적·GE 수가 늘면 대역폭 비용을 재고 UI 표시용 최소 데이터만 복제하도록 줄이는 것을 고려할 것.
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

	NameplateComponent->InitializeViewModels(AbilitySystemComponent, GetCharacterUIData());
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

	// 외형 없는 재화(골드 등)는 로컬 플레이어 인벤토리에 즉시 지급한다.
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UWxRewardLibrary::GrantReward(this, RewardRow, PlayerController, GetActorTransform(), FVector::UpVector * LaunchSpeed);
	}
}

bool AWxEnemyCharacter::IsInteractionMeshActive(const UPrimitiveComponent* InMesh) const
{
	// 영역은 늘 열어 두고, 실제 자격은 CanBeInteractedBy 가 주체별로 판정한다.
	// 이 판정은 머신당 답이 하나뿐이라 특정 플레이어에 종속시킬 수 없다 — 그렇게 하면 서버 답이 한 플레이어 기준이 되어 다른 플레이어의 정당한 처형이 거부된다.
	return InMesh == GetMesh();
}

bool AWxEnemyCharacter::CanBeInteractedBy(const AActor* Interactor, const UActorComponent* Source) const
{
	// 처형 자격은 주체별로 갈린다(뒤잡은 주체가 후방 원뿔 안에 있어야 한다) — 채널로는 표현할 수 없어 여기서 판정한다.
	// 외곽선은 스캐너가 선택 대상에만 켠다.
	return GetEligibleFinisherEventTag(Interactor).IsValid();
}

FGameplayTag AWxEnemyCharacter::GetEligibleFinisherEventTag(const AActor* Interactor) const
{
	if (!IsAlive() || !AbilitySystemComponent)
	{
		return FGameplayTag();
	}

	// 이미 처형 연출 중이면 자격 없음 — 공격자 어빌리티(WxAbility_Finisher)가 연출 동안 대상에 State.Finisher 를 걸어 둔다.
	// 노출과 발동 검증이 같은 함수를 지나므로, 연출 중 재노출도 다른 플레이어의 중복 발동도 여기서 함께 막힌다.
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Finisher))
	{
		return FGameplayTag();
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		return WxGameplayTags::Event_Finisher;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat) && Interactor)
	{
		FVector ToInteractor = Interactor->GetActorLocation() - GetActorLocation();
		ToInteractor.Z = 0.0;
		if (ToInteractor.Normalize())
		{
			// 정면 내적이 후방 임계 이하 = 주체가 후방 원뿔 안. (반각 90° → 임계 0 → 후방 반구)
			const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToInteractor);
			const float RearThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabRearHalfAngle));
			if (ForwardDot <= RearThreshold)
			{
				return WxGameplayTags::Event_Backstab;
			}
		}
	}

	return FGameplayTag();
}

void AWxEnemyCharacter::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다.
	if (!Interactor)
	{
		return;
	}

	// 발동 변형은 실제 상호작용 주체(Interactor) 기준으로 발동 시점에 다시 정한다(노출~발동 사이 상태·위치 변화를 흡수, 서버 권위 검증).
	const FGameplayTag EventTag = GetEligibleFinisherEventTag(Interactor);
	if (!EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = EventTag;
	// 이 호출로 처형 어빌리티가 동기 트리거되어 대상(this)에 State.Finisher 가 붙는다 — 재노출·중복 발동 차단에 별도 래치가 필요 없다.
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, EventTag, EventData);
}

FText AWxEnemyCharacter::GetInteractionPrompt(const UActorComponent* Source) const
{
	return FText::FromString(TEXT("Finisher"));
}

void AWxEnemyCharacter::OnSpawnedBy(AWxSpawner* Spawner)
{
	OwningSpawner = Spawner;
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

