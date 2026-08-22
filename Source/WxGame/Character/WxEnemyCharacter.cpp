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

FGameplayTag AWxEnemyCharacter::GetEligibleFinisherEventTag(const AActor* Interactor) const
{
	if (!IsAlive())
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

bool AWxEnemyCharacter::CanInteract() const
{
	// TODO: PlayerCharacter가 뒤에서 접근해서 백스탭 가능해야함. GetEligibleFinisherEventTag 함수도 제거해야함.
	return HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy);
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

