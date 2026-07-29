// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "Inventory/WxEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Weapon/WxWeaponBase.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxCharacterBase::AWxCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
	GetCharacterMovement()->AirControl = 0.35f;
}

void AWxCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// ModularGameplay 컴포넌트 수신 opt-in. 폰 대상 주입 요청(Experience 액션)의 컴포넌트가 여기에 자동 부착된다.
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

void AWxCharacterBase::HandleRagdollTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		EnterRagdoll();
	}
}

void AWxCharacterBase::EnterRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	// Ragdoll 프로필이 Camera 응답을 Block으로 덮어쓰므로, 스프링암 카메라가 래그돌 본에 걸려 줌-인되는 현상을 방지한다.
	MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComp->SetSimulatePhysics(true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	// 소켓 변경은 WeaponActor(ChildActorComponent) 를 현재 부모 컴포넌트의 새 소켓으로 재부착한다.
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
