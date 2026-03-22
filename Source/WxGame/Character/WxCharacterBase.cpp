// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "WxCollisionChannels.h"
#include "Weapon/WxWeaponBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AWxCharacterBase::AWxCharacterBase()
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(WxCollision::Attack, ECR_Overlap);
	
	AbilitySystemComponent = CreateDefaultSubobject<UWxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AttributeSet = CreateDefaultSubobject<UWxCombatAttributeSet>(TEXT("AttributeSet"));

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

void AWxCharacterBase::SpawnDefaultWeapon()
{
	if (!DefaultWeaponClass || !HasAuthority())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	EquippedWeapon = GetWorld()->SpawnActor<AWxWeaponBase>(DefaultWeaponClass, SpawnParams);
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachToCharacter(this, WeaponSocketName);
	}
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
		AbilitySystemComponent->AddLooseGameplayTag(WxGameplayTags::State_Airborne);
	}
	else if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Airborne))
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(WxGameplayTags::State_Airborne);
	}
}

void AWxCharacterBase::HandleDeath()
{
	OnDeath.Broadcast(this);
}
