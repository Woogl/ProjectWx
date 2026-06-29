// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Component/WxNameplateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxRewardLibrary.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Interaction/WxInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Spawnable/WxSpawner.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxGameplayTags.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// Full 모드: 모든 GE를 모든 클라이언트에 복제한다. 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
	// 적이 많아지거나 GE 수가 늘어날 경우 대역폭 비용을 측정하고, 필요하면 아래 방향으로 개선을 고려할 것:
	//   - UI 표시용 최소 데이터(Icon, Duration, StartWorldTime)만 담는 경량 복제 컴포넌트로 교체
	//   - UI에 표시할 이펙트에만 UWxEffectComponent_UIData를 붙이는 규칙을 엄수해 복제 대상 GE 수를 제한
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	NameplateComponent = CreateDefaultSubobject<UWxNameplateComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	// 락온 지점을 메시에 부착한다.
	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));

	// 처형 상호작용 볼륨. 평소엔 BeginPlay 에서 비활성화하고, 조건(그로기 또는 미인지·후방)을 주기 평가해 켠다.
	// 메시에 부착해 처형 가능 시 Gimmick과 동일한 외곽선(Custom Depth/Stencil)이 적 몸체에 적용되게 한다.
	FinisherInteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("FinisherInteractionComponent"));
	FinisherInteractionComponent->SetupAttachment(GetMesh());
	FinisherInteractionComponent->SetInteractionText(FText::FromString(TEXT("Finisher")));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	NameplateComponent->InitializeViewModels(AbilitySystemComponent, GetCharacterUIData());

	// 처형 상호작용은 조건(그로기=앞잡 / 미인지·후방=뒤잡)을 권위에서 주기적으로 평가해 노출한다.
	FinisherInteractionComponent->SetInteractionEnabled(false);
	FinisherInteractionComponent->OnInteracted.AddDynamic(this, &AWxEnemyCharacter::HandleFinisherInteracted);
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(FinisherAffordanceTimerHandle, this, &AWxEnemyCharacter::UpdateFinisherAffordance, 0.15f, true);
	}
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	if (!HasAuthority())
	{
		return;
	}

	// 사망 시 처형 어포던스 갱신을 멈춘다.
	GetWorldTimerManager().ClearTimer(FinisherAffordanceTimerHandle);

	if (AWxSpawner* Spawner = OwningSpawner.Get())
	{
		Spawner->MarkKilled();
	}

	// 외형 없는 재화(골드 등)는 로컬 플레이어 인벤토리에 즉시 지급한다.
	// 외형 있는 보상은 사망 위치에서 월드 Z 업으로 수직 발사된다(스칼라 LaunchSpeed 를 업 벡터로 올려 전달).
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UWxRewardLibrary::GrantReward(this, RewardRow, PlayerController, GetActorTransform(), FVector::UpVector * LaunchSpeed);
	}
}

void AWxEnemyCharacter::UpdateFinisherAffordance()
{
	// 노출은 발동과 동일한 단일 평가를 따른다. 발동 가능한 변형이 있을 때만 상호작용을 노출한다.
	FinisherInteractionComponent->SetInteractionEnabled(GetEligibleFinisherEventTag().IsValid());
}

FGameplayTag AWxEnemyCharacter::GetEligibleFinisherEventTag() const
{
	if (!IsAlive() || !AbilitySystemComponent)
	{
		return FGameplayTag();
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		// 앞잡: 그로기 상태면 방향과 무관하게 가능하다.
		return WxGameplayTags::Event_Finisher;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat))
	{
		// 뒤잡: 미인지(비전투) 상태에서 로컬 플레이어가 후방 원뿔 안에 있을 때만 가능하다.
		if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
			ToPlayer.Z = 0.0;
			if (ToPlayer.Normalize())
			{
				// 정면 내적이 후방 임계 이하 = 플레이어가 후방 원뿔 안. (반각 90° → 임계 0 → 후방 반구)
				const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToPlayer);
				const float RearThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabRearHalfAngle));
				if (ForwardDot <= RearThreshold)
				{
					return WxGameplayTags::Event_Backstab;
				}
			}
		}
	}

	return FGameplayTag();
}

void AWxEnemyCharacter::HandleFinisherInteracted(AActor* InstigatorActor)
{
	if (!HasAuthority() || !InstigatorActor)
	{
		return;
	}

	// 발동 변형은 노출과 동일한 평가로 발동 시점에 다시 정한다(노출~발동 사이 상태 변화를 흡수).
	// 자격이 없으면(어포던스 없음) 아무것도 발동하지 않는다.
	const FGameplayTag EventTag = GetEligibleFinisherEventTag();
	if (!EventTag.IsValid())
	{
		return;
	}

	// 공격자(플레이어) ASC 로 처형 발동 이벤트를 보낸다. 대상(this)은 EventData.Target 으로 전달된다.
	FGameplayEventData EventData;
	EventData.Instigator = InstigatorActor;
	EventData.Target = this;
	EventData.EventTag = EventTag;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InstigatorActor, EventTag, EventData);
}

void AWxEnemyCharacter::OnSpawnedBy(AWxSpawner* Spawner)
{
	OwningSpawner = Spawner;
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

float AWxEnemyCharacter::GetSightRadius() const
{
	return SightRadius;
}

float AWxEnemyCharacter::GetSightAngle() const
{
	return SightAngle;
}

float AWxEnemyCharacter::GetMaxHearingRange() const
{
	return MaxHearingRange;
}

#if WITH_EDITOR
const UMeshComponent* AWxEnemyCharacter::GetEditorPreviewMeshComponent() const
{
	return GetMesh();
}
#endif

