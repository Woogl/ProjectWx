// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Component/WxNameplateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxRewardLibrary.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Damage.h"
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

void AWxEnemyCharacter::InitAbilitySystem()
{
	Super::InitAbilitySystem();

	// AI 폰의 빙의는 서버에서만 일어나므로 이 구독도 서버 전용이다 — AI Perception 이 도는 곳과 같다.
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetIncomingDamageAttribute())
		.AddUObject(this, &AWxEnemyCharacter::HandleIncomingDamageChanged);
}

void AWxEnemyCharacter::HandleIncomingDamageChanged(const FOnAttributeChangeData& Data)
{
	// 메타 어트리뷰트가 GE 실행으로 바뀔 때만 가해자·적중 지점이 실린 콜백 데이터가 함께 온다.
	// 어트리뷰트셋이 소비하며 되돌리는 0 쓰기에는 실리지 않는다.
	if (Data.NewValue <= 0.f || !Data.GEModData)
	{
		return;
	}

	const FGameplayEffectContextHandle Context = Data.GEModData->EffectSpec.GetContext();
	AActor* DamageInstigator = Context.GetInstigator();
	if (!DamageInstigator)
	{
		return;
	}

	// EventLocation 으로 넘긴 가해자 위치가 그대로 Stimulus 위치가 된다.
	const FVector HitLocation = Context.GetHitResult() ? FVector(Context.GetHitResult()->ImpactPoint) : GetActorLocation();
	UAISense_Damage::ReportDamageEvent(this, this, DamageInstigator, Data.NewValue, DamageInstigator->GetActorLocation(), HitLocation);
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

bool AWxEnemyCharacter::IsInteractionEnabled() const
{
	// 늘 열어 두고, 실제 자격은 CanBeInteractedBy 가 주체별로 판정한다.
	// 이 판정은 머신당 답이 하나뿐이라 특정 플레이어에 종속시킬 수 없다 — 그렇게 하면 서버 답이 한 플레이어 기준이 되어 다른 플레이어의 정당한 처형이 거부된다.
	return true;
}

bool AWxEnemyCharacter::CanBeInteractedBy(const AActor* Interactor) const
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

	// 공격자 어빌리티(WxAbility_Finisher)가 연출 동안 대상에 State.BeingFinished 를 걸어 둔다.
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_BeingFinished))
	{
		return FGameplayTag();
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		return WxGameplayTags::Event_Finisher;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat) && Interactor)
	{
		FVector ToInteractor = Interactor->GetActorLocation() - GetActorLocation();
		ToInteractor.Z = 0.0;
		if (ToInteractor.Normalize())
		{
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

void AWxEnemyCharacter::OnInteracted(AActor* Interactor)
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
	// 이 호출로 처형 어빌리티가 동기 트리거되어 대상(this)에 State.BeingFinished 가 붙는다 — 재노출·중복 발동 차단에 별도 래치가 필요 없다.
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

