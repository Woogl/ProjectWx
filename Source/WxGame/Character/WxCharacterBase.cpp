// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "Character/WxCharacterMovementComponent.h"
#include "Character/WxMetaHumanVisualComponent.h"
#include "Inventory/WxEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxCharacterBase::AWxCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWxCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Ignore);
	
	AbilitySystemComponent = CreateDefaultSubobject<UWxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	CombatAttributeSet = CreateDefaultSubobject<UWxCombatAttributeSet>(TEXT("CombatAttributeSet"));

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	EquipmentComponent = CreateDefaultSubobject<UWxEquipmentComponent>(TEXT("EquipmentComponent"));

	WeaponActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("WeaponActor"));
	WeaponActor->SetupAttachment(GetMesh(), TEXT("hand_r"));

	MetaHumanVisualComponent = CreateDefaultSubobject<UWxMetaHumanVisualComponent>(TEXT("MetaHumanVisualComponent"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
}

void AWxCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 래그돌 감지는 시뮬 프록시를 포함한 전 머신에서 필요하므로, 서버·오너 클라에서만 도는 InitAbilitySystem이 아니라 여기서 구독한다.
	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Ragdoll, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AWxCharacterBase::HandleRagdollTagChanged);

	// late join 시 구독보다 먼저 초기 복제로 태그가 실려 왔을 수 있어 1회 즉시 확인한다.
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Ragdoll))
	{
		EnterRagdoll();
	}

	// 사망 처리도 같은 이유로 여기서 구독한다 — 무기 판정 해제와 OnDeath 방송은 시뮬 프록시를 포함한 전 머신에서 일어나야 한다.
	// 보상 지급 같은 권위 전용 처리는 HandleDeath 내부의 HasAuthority 가드가 계속 가른다.
	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AWxCharacterBase::HandleDeathTagChanged);

	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
	{
		HandleDeath();
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquipVisualChanged.AddUObject(this, &AWxCharacterBase::HandleEquipVisualChanged);
	}

	if (!WeaponActor)
	{
		return;
	}
	
	if (AActor* SpawnedWeapon = WeaponActor->GetChildActor())
	{
		SpawnedWeapon->SetOwner(this);
	}
}

void AWxCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}

void AWxCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilitySystem();
}

bool AWxCharacterBase::CanJumpInternal_Implementation() const
{
	if (AbilitySystemComponent)
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
		{
			return false;
		}

		// 액션 어빌리티(Attack/Dodge/Skill/Ultimate/Guard 등)는 활성 동안 액션 마커를 차단하므로, 그 차단 여부로 어빌리티 발동 중인지 판별해 점프를 막는다.
		// 후딜 캔슬 구간에서 차단이 풀리면 다른 캔슬 액션과 동일하게 점프도 허용된다.
		if (AbilitySystemComponent->AreAbilityTagsBlocked(FGameplayTagContainer(WxGameplayTags::Ability_Exclusive)))
		{
			return false;
		}
	}

	return Super::CanJumpInternal_Implementation();
}

UAbilitySystemComponent* AWxCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AWxCharacterBase::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetOwnedGameplayTags(TagContainer);
	}
}

AWxWeaponBase* AWxCharacterBase::GetEquippedWeapon() const
{
	return WeaponActor ? Cast<AWxWeaponBase>(WeaponActor->GetChildActor()) : nullptr;
}

const FWxCharacterUIData& AWxCharacterBase::GetCharacterUIData() const
{
	return UIData;
}

void AWxCharacterBase::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
	Team = static_cast<EWxTeam>(InTeamId.GetId());
}

FGenericTeamId AWxCharacterBase::GetGenericTeamId() const
{
	return FGenericTeamId(static_cast<uint8>(Team));
}

ETeamAttitude::Type AWxCharacterBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other))
	{
		if (Team == EWxTeam::Neutral || static_cast<EWxTeam>(OtherTeamAgent->GetGenericTeamId().GetId()) == EWxTeam::Neutral)
		{
			return ETeamAttitude::Neutral;
		}
		return GetGenericTeamId() == OtherTeamAgent->GetGenericTeamId() ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
	}
	return ETeamAttitude::Neutral;
}

bool AWxCharacterBase::IsAlive() const
{
	if (const UWxCombatAttributeSet* AttrSet = AbilitySystemComponent->GetSet<UWxCombatAttributeSet>())
	{
		return AttrSet->GetHP() > 0.f;
	}
	return false;
}

void AWxCharacterBase::InitAbilitySystem()
{
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	AbilitySystemComponent->RefreshAbilityActorInfo();

	// GiveAbilitySet보다 먼저 등록해야 초기 어트리뷰트 변경(SPD 등)이 콜백에 반영됨
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetSPDAttribute())
		.AddUObject(this, &AWxCharacterBase::HandleSPDAttributeChanged);

	// GiveAbility는 서버에서만 허용. 클라이언트에는 서버로부터 복제됨
	if (HasAuthority())
	{
		AbilitySystemComponent->GiveAbilitySet();
	}
}

void AWxCharacterBase::HandleSPDAttributeChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * Data.NewValue;
}

void AWxCharacterBase::HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		HandleDeath();
	}
}

void AWxCharacterBase::HandleRagdollTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		EnterRagdoll();
	}
}

void AWxCharacterBase::EnterRagdoll()
{
	// 메시 등록 해제 도중 AnimInstance가 몽타주를 강제 종료하면 사망 어빌리티의 인터럽트 경로가 여기로 재진입한다.
	// 그때는 bRegistered가 아직 참이라 물리 상태가 새로 생기고, 곧 등록만 풀려 orphan으로 남는다.
	const UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || IsActorBeingDestroyed())
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	// Ragdoll 프로필이 응답 컨테이너를 통째로 덮으므로, 사망 시 걸어둔 override를 다시 적용한다.
	// Camera는 Block으로 덮이면 스프링암 카메라가 래그돌 본에 걸려 줌-인되고, WxAttack은 Block으로 덮이면 시체가 다시 맞는다.
	MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Ignore);

	// 전자는 모든 바디를 시뮬로 돌리고, 후자는 bBlendPhysics를 켜되 PhysType_Default 바디만 건드리므로 둘 다 필요하다.
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}

void AWxCharacterBase::HandleEquipVisualChanged(USkeletalMesh* MeshAsset, FName Socket)
{
	AWxWeaponBase* Weapon = GetEquippedWeapon();
	if (!Weapon)
	{
		// ChildActor 가 아직 스폰되지 않았을 수 있다(초기 복제 타이밍).
		// 다음 방송 사이클에 재시도된다.
		return;
	}

	Weapon->SetVisualMesh(MeshAsset);

	if (WeaponActor && Socket != NAME_None)
	{
		if (USceneComponent* CurrentParent = WeaponActor->GetAttachParent())
		{
			WeaponActor->AttachToComponent(CurrentParent, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
		}
	}
}

void AWxCharacterBase::HandleDeath()
{
	// 스윙 도중 죽으면 공격 구간을 닫을 ANS 종료가 오지 않을 수 있으므로, 시체의 무기가 계속 때리지 않도록 여기서 판정을 걷어낸다.
	// 사망 태그는 복제되어 모든 머신에서 이 경로를 타므로, 각 머신의 로컬 판정이 함께 해제된다.
	if (AWxWeaponBase* Weapon = GetEquippedWeapon())
	{
		Weapon->CancelAttack();
	}

	OnDeath.Broadcast(this);
}
