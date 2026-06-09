// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "Character/WxCharacterMovementComponent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "Component/WxEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxCharacterBase::AWxCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWxCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(WxCollision::WxAttack, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(WxCollision::WxAttack, ECR_Ignore);
	
	AbilitySystemComponent = CreateDefaultSubobject<UWxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	CombatAttributeSet = CreateDefaultSubobject<UWxCombatAttributeSet>(TEXT("CombatAttributeSet"));

	LockOnComponent = CreateDefaultSubobject<UWxLockOnComponent>(TEXT("LockOnComponent"));

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	EquipmentComponent = CreateDefaultSubobject<UWxEquipmentComponent>(TEXT("EquipmentComponent"));

	WeaponActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("WeaponActor"));
	WeaponActor->SetupAttachment(GetMesh(), TEXT("hand_r"));
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;

	// 이동 속도·가감속 디폴트
	GetCharacterMovement()->MaxAcceleration = 1500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	GetCharacterMovement()->BrakingFrictionFactor = 1.f;

	// 점프·낙하 디폴트
	GetCharacterMovement()->JumpZVelocity = 420.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->GravityScale = 1.5f;
}

void AWxCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!WeaponActor)
	{
		return;
	}
	
	if (AActor* SpawnedWeapon = WeaponActor->GetChildActor())
	{
		SpawnedWeapon->SetOwner(this);
	}

	// "VisualOverride" 태그가 붙은 외형 SkeletalMeshComponent 가 캐릭터 메시 하위에 있으면 무기 부착 대상을 그쪽으로 옮긴다.
	// 동일 스켈레톤을 공유하는 외형 오버라이드 메시 위 소켓에 무기가 따라가도록 보장하기 위함.
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	for (USceneComponent* Child : CharacterMesh->GetAttachChildren())
	{
		USkeletalMeshComponent* SkelChild = Cast<USkeletalMeshComponent>(Child);
		if (SkelChild && SkelChild->ComponentHasTag(TEXT("VisualOverride")))
		{
			WeaponActor->AttachToComponent(SkelChild, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponActor->GetAttachSocketName());
			break;
		}
	}
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
		// 사망 상태에서는 점프 불가
		if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
		{
			return false;
		}

		// 액션 어빌리티(Attack/Dodge/Skill/Ultimate/Guard 등)는 활성 동안 Ability 태그를 차단하므로, 그 차단 여부로 어빌리티 발동 중인지 판별해 점프를 막는다.
		// 후딜 캔슬 구간에서 차단이 풀리면 다른 캔슬 액션과 동일하게 점프도 허용된다.
		if (AbilitySystemComponent->AreAbilityTagsBlocked(FGameplayTagContainer(WxGameplayTags::Ability)))
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

void AWxCharacterBase::EquipItem(const UWxItemDefinition* ItemDef)
{
	if (EquipmentComponent)
	{
		EquipmentComponent->EquipItem(ItemDef);
	}
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

	// InitializeAbilities보다 먼저 등록해야 초기 어트리뷰트 변경(SPD 등)이 콜백에 반영됨
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetSPDAttribute())
		.AddUObject(this, &AWxCharacterBase::HandleSPDAttributeChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AWxCharacterBase::HandleDeathTagChanged);

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

void AWxCharacterBase::HandleDeath()
{
	OnDeath.Broadcast(this);
}
