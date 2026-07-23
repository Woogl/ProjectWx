// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Component/WxNameplateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxRewardLibrary.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WxCollisionChannels.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Spawnable/WxSpawner.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxBGMSourceComponent.h"
#include "WxGameplayTags.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// Full 모드: 모든 GE를 모든 클라이언트에 복제한다.
	// 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
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


	// 상태 기반 BGM 소스.
	// 실제 태그·우선순위는 각 적·보스 BP 에서 설정한다(MusicTag 를 비우면 inert).
	BGMSourceComponent = CreateDefaultSubobject<UWxBGMSourceComponent>(TEXT("BGMSourceComponent"));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	NameplateComponent->InitializeViewModels(AbilitySystemComponent, GetCharacterUIData());

	// 처형 상호작용 영역은 캐릭터 메시 자체다. 시작 시 꺼두고, 조건(그로기=앞잡 / 미인지·후방=뒤잡)을 어포던스 타이머가 주기 평가해 켠다.
	// 각 머신이 로컬로 평가한다 — 판정 입력(HP·상태 태그·트랜스폼)이 전부 복제되므로 복제 없이 같은 값에 수렴하고, 클라는 자기 로컬 플레이어 기준으로 노출을 판정한다.
	GetMesh()->SetCollisionResponseToChannel(ECC_WxInteractable, ECR_Ignore);
	GetWorldTimerManager().SetTimer(FinisherAffordanceTimerHandle, this, &AWxEnemyCharacter::UpdateFinisherAffordance, 0.15f, true);
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	// 사망 시 처형 어포던스 갱신을 멈춘다(전 머신에서 구동되므로 권위 가드 바깥이다).
	GetWorldTimerManager().ClearTimer(FinisherAffordanceTimerHandle);

	if (!HasAuthority())
	{
		return;
	}

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
	// 노출은 이 머신의 로컬 플레이어 기준으로 평가한다. 발동 검증은 실제 instigator 를 쓰므로(OnInteracted) 노출~발동 주체가 갈려도 발동이 서버 권위 최종 판정이다.
	const bool bEligible = GetEligibleFinisherEventTag(UGameplayStatics::GetPlayerPawn(this, 0)).IsValid();

	// 처형 가능하면 메시를 스캔에 노출한다. 외곽선은 레지스트리가 선택 대상에만 켠다.
	GetMesh()->SetCollisionResponseToChannel(ECC_WxInteractable, bEligible ? ECR_Overlap : ECR_Ignore);
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
		// 앞잡: 그로기 상태면 방향과 무관하게 가능하다(주체 위치 불필요).
		return WxGameplayTags::Event_Finisher;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat) && Interactor)
	{
		// 뒤잡: 미인지(비전투) 상태에서 상호작용 주체가 후방 원뿔 안에 있을 때만 가능하다.
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
	// 자격이 없으면(후방 아님 등) 아무것도 발동하지 않는다.
	const FGameplayTag EventTag = GetEligibleFinisherEventTag(Interactor);
	if (!EventTag.IsValid())
	{
		return;
	}

	// 공격자(플레이어) ASC 로 처형 발동 이벤트를 보낸다.
	// 대상(this)은 EventData.Target 으로 전달된다.
	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = EventTag;
	// 이 호출로 처형 어빌리티가 동기 트리거되어 대상(this)에 State.Finisher 가 붙는다.
	// 그 시점부터 GetEligibleFinisherEventTag 가 자격을 거부하므로, 재노출·중복 발동 차단에 별도 래치가 필요 없다.
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, EventTag, EventData);
}

FText AWxEnemyCharacter::GetInteractionPrompt() const
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

