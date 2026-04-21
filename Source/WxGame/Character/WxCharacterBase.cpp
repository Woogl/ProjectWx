// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "Targeting/WxLockOnComponent.h"
#include "MotionWarpingComponent.h"
#include "WxGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "WxCollisionChannels.h"
#include "Weapon/WxWeaponBase.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AWxCharacterBase::AWxCharacterBase()
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

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
}

void AWxCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxCharacterBase, EquippedWeapon);
	DOREPLIFETIME(AWxCharacterBase, EquippedItemDef);
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
	return EquippedWeapon;
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

void AWxCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilitySystem();
}

void AWxCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
}

void AWxCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AWxCharacterBase::SpawnDefaultWeapon()
{
	if (!WeaponActor || !HasAuthority())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	EquippedWeapon = GetWorld()->SpawnActor<AWxWeaponBase>(WeaponActor, SpawnParams);
	if (EquippedWeapon)
	{
		// 서버 BeginPlay 이후 이미 EquippedItemDef가 설정돼 있을 수도 있으므로, 스폰 직후 장착 적용.
		ApplyEquipmentVisuals();
	}
}

void AWxCharacterBase::EquipItem(const UWxItemDefinition* ItemDef)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ItemDef && !ItemDef->FindFragment<FWxItemFragment_Equipment>())
	{
		return;
	}

	EquippedItemDef = ItemDef;
	ApplyEquipmentVisuals();
}

void AWxCharacterBase::OnRep_EquippedWeapon()
{
	// 늦게 도착한 EquippedWeapon이 기존 EquippedItemDef를 따라 시각에 반영되도록 재적용.
	ApplyEquipmentVisuals();
}

void AWxCharacterBase::OnRep_EquippedItemDef()
{
	ApplyEquipmentVisuals();
}

void AWxCharacterBase::ApplyEquipmentVisuals()
{
	if (!EquippedWeapon)
	{
		return;
	}

	// EquippedItemDef가 없으면 CDO 기본 메시 + 기본 소켓으로 장착 (장착 해제 상태).
	FName Socket = DefaultWeaponSocket;
	USkeletalMesh* MeshAsset = nullptr;

	if (EquippedItemDef)
	{
		if (const FWxItemFragment_Equipment* Fragment = EquippedItemDef->FindFragment<FWxItemFragment_Equipment>())
		{
			Socket = Fragment->AttachSocket;
			MeshAsset = Fragment->SkeletalMesh;
		}
	}

	// BP에서 추가한 외형 SkeletalMesh가 있으면 그 메시에 장착, 없으면 GetMesh()에 직접 장착.
	USkeletalMeshComponent* Visual = GetMesh();
	for (USceneComponent* Child : Visual->GetAttachChildren())
	{
		if (USkeletalMeshComponent* SkelChild = Cast<USkeletalMeshComponent>(Child))
		{
			Visual = SkelChild;
			break;
		}
	}
	EquippedWeapon->Equip(Visual, Socket, MeshAsset);
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

void AWxCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (!AbilitySystemComponent)
	{
		return;
	}

	const EMovementMode CurrentMode = GetCharacterMovement()->MovementMode;
	const bool bIsAirborne = (CurrentMode == MOVE_Falling || CurrentMode == MOVE_Flying);

	if (bIsAirborne)
	{
		AbilitySystemComponent->AddLooseGameplayTag(WxGameplayTags::State_Aerial);
	}
	else if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Aerial))
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(WxGameplayTags::State_Aerial);
	}
}

void AWxCharacterBase::HandleDeath()
{
	OnDeath.Broadcast(this);
}
