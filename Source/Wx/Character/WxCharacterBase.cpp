// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxAttributeSet.h"
#include "AbilitySystem/WxGameplayAbility.h"
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

UWxAttributeSet* AWxCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

AWxWeaponBase* AWxCharacterBase::GetEquippedWeapon() const
{
	return EquippedWeapon;
}

bool AWxCharacterBase::IsAlive() const
{
	return AttributeSet && AttributeSet->GetHP() > 0.f;
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
	ApplyDefaultEffects();
	GiveDefaultAbilities();

	// SPD 변경 시 실제 이동 속도에 반영
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxAttributeSet::GetSPDAttribute())
		.AddUObject(this, &AWxCharacterBase::HandleSPDAttributeChanged);

	// State_Dead 태그 부여 시 사망 처리
	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AWxCharacterBase::HandleDeathTagChanged);
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

void AWxCharacterBase::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1);
			if (const UWxGameplayAbility* DefaultAbility = Cast<UWxGameplayAbility>(AbilityClass.GetDefaultObject()))
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

void AWxCharacterBase::ApplyDefaultEffects()
{
	if (!AbilitySystemComponent || DefaultEffects.IsEmpty())
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& DefaultEffect : DefaultEffects)
	{
		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DefaultEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void AWxCharacterBase::HandleDeath()
{
	OnDeath.Broadcast(this);
	RagdollComponent->EnableRagdoll();
}
