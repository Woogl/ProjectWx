// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxAttributeSet.h"
#include "Component/WxRagdollComponent.h"
#include "WxGameplayTags.h"
#include "Weapon/WxWeaponBase.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxCharacterBase::AWxCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UWxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UWxAttributeSet>(TEXT("AttributeSet"));

	RagdollComponent = CreateDefaultSubobject<UWxRagdollComponent>(TEXT("RagdollComponent"));
}

UAbilitySystemComponent* AWxCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UWxAbilitySet* AWxCharacterBase::GetAbilitySet() const
{
	return AbilitySet;
}

AWxWeaponBase* AWxCharacterBase::GetEquippedWeapon() const
{
	return EquippedWeapon;
}

bool AWxCharacterBase::IsAlive() const
{
	if (const UWxAttributeSet* AttrSet = AbilitySystemComponent->GetSet<UWxAttributeSet>())
	{
		return AttrSet->GetHP() > 0.f;
	}
	return false;
}

void AWxCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
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

void AWxCharacterBase::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// GiveAbilitySet보다 먼저 등록해야 초기 어트리뷰트 변경(SPD 등)이 콜백에 반영됨
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxAttributeSet::GetSPDAttribute())
		.AddUObject(this, &AWxCharacterBase::HandleSPDAttributeChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AWxCharacterBase::HandleDeathTagChanged);

	GiveAbilitySet();
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
	else
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(WxGameplayTags::State_Airborne);
	}
}

void AWxCharacterBase::GiveAbilitySet()
{
	if (!AbilitySystemComponent || !AbilitySet)
	{
		return;
	}

	AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &AbilitySetGrantedHandles);
}

void AWxCharacterBase::HandleDeath()
{
	OnDeath.Broadcast(this);
	RagdollComponent->EnableRagdoll();
}
