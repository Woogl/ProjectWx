// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxAttributeSet.h"
#include "Ability/WxGameplayAbility.h"
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

bool AWxCharacterBase::IsAlive() const
{
	return AttributeSet && AttributeSet->GetHP() > 0.f;
}

void AWxCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	SpawnDefaultWeapon();
}

void AWxCharacterBase::SpawnDefaultWeapon()
{
	if (!DefaultWeaponClass || !HasAuthority()) return;

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
	ApplyDefaultAttributes();
	GiveDefaultAbilities();

	// SPD 변경 시 실제 이동 속도에 반영
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxAttributeSet::GetSPDAttribute())
		.AddUObject(this, &AWxCharacterBase::HandleSPDAttributeChanged);
}

void AWxCharacterBase::HandleSPDAttributeChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * Data.NewValue;
}

void AWxCharacterBase::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent) return;

	for (const TSubclassOf<UWxGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1);
			if (const UWxGameplayAbility* DefaultAbility = AbilityClass.GetDefaultObject())
			{
				if (DefaultAbility->ActivationInputTag.IsValid())
				{
					Spec.GetDynamicSpecSourceTags().AddTag(DefaultAbility->ActivationInputTag);
				}
			}
			AbilitySystemComponent->GiveAbility(Spec);
		}
	}
}

void AWxCharacterBase::ApplyDefaultAttributes()
{
	if (!AbilitySystemComponent || !DefaultAttributeEffect) return;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void AWxCharacterBase::HandleDeath()
{
	AbilitySystemComponent->AddLooseGameplayTag(WxGameplayTags::State_Dead);
	OnDeath.Broadcast(this);

	RagdollComponent->EnableRagdoll();
}
