// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/WxCharacterBase.h"
#include "GAS/WxAbilitySystemComponent.h"
#include "GAS/WxAttributeSet.h"
#include "GAS/WxGameplayAbility.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxCharacterBase::AWxCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UWxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UWxAttributeSet>(TEXT("AttributeSet"));

	DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
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
}

void AWxCharacterBase::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyDefaultAttributes();
	GiveDefaultAbilities();

	// SPD 변경 시 실제 이동 속도에 반영
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UWxAttributeSet::GetSPDAttribute())
		.AddUObject(this, &AWxCharacterBase::OnSPDAttributeChanged);
}

void AWxCharacterBase::OnSPDAttributeChanged(const FOnAttributeChangeData& Data)
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
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
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
	AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	OnDeath.Broadcast(this);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->Velocity     = FVector::ZeroVector;
}
