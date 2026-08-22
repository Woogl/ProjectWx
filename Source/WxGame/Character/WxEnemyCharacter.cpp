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

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetIncomingDamageAttribute())
		.AddUObject(this, &AWxEnemyCharacter::HandleIncomingDamageChanged);
}

void AWxEnemyCharacter::HandleIncomingDamageChanged(const FOnAttributeChangeData& Data)
{
	// IncomingDamage 메타 어트리뷰트로 피해를 받을 때만 적용
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

	// 가해자 위치가 Stimulus 위치
	const FVector HitLocation = Context.GetHitResult() ? FVector(Context.GetHitResult()->ImpactPoint) : GetActorLocation();
	UAISense_Damage::ReportDamageEvent(this, this, DamageInstigator, Data.NewValue, DamageInstigator->GetActorLocation(), HitLocation);
}

bool AWxEnemyCharacter::IsLocalPlayerInRearCone() const
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return false;
	}

	FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.0;
	if (!ToPlayer.Normalize())
	{
		return false;
	}

	const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToPlayer);
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


	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		// 처치 보상 지급
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

	// 그로기 앞잡(Finisher) 인지, 뒤잡(Backstab)인지 판별
	const FGameplayTag EventTag = AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy)
		? WxGameplayTags::Event_Finisher
		: WxGameplayTags::Event_Backstab;

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = EventTag;

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
	if (!IsAlive())
	{
		return false;
	}

	// 공격자 어빌리티(WxAbility_Finisher)가 연출 동안 대상에 State.BeingFinished 를 걸어 둔다.
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_BeingFinished))
	{
		return false;
	}

	// 그로기는 이미 무방비라 방향을 묻지 않는다(앞잡).
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		return true;
	}

	// 뒤잡은 적이 아직 나를 인지하지 못했을 때만 성립한다.
	return !AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat) && IsLocalPlayerInRearCone();
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

