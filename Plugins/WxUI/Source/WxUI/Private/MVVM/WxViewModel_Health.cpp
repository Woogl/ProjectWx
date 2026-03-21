// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Health.h"
#include "AbilitySystemComponent.h"

void UWxViewModel_Health::Initialize(UAbilitySystemComponent* InASC, FGameplayAttribute InHPAttribute, FGameplayAttribute InMaxHPAttribute)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;

	if (InHPAttribute.IsValid())
	{
		BoundHPAttribute = InHPAttribute;
		SetCurrentHP(InASC->GetNumericAttribute(InHPAttribute));
		InASC->GetGameplayAttributeValueChangeDelegate(InHPAttribute)
			.AddUObject(this, &UWxViewModel_Health::HandleHPChanged);
	}

	if (InMaxHPAttribute.IsValid())
	{
		BoundMaxHPAttribute = InMaxHPAttribute;
		SetMaxHP(InASC->GetNumericAttribute(InMaxHPAttribute));
		InASC->GetGameplayAttributeValueChangeDelegate(InMaxHPAttribute)
			.AddUObject(this, &UWxViewModel_Health::HandleMaxHPChanged);
	}

	RecalculateHealthPercent();
}

void UWxViewModel_Health::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (BoundHPAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BoundHPAttribute).RemoveAll(this);
		}

		if (BoundMaxHPAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BoundMaxHPAttribute).RemoveAll(this);
		}
	}

	CachedASC.Reset();
	BoundHPAttribute = FGameplayAttribute();
	BoundMaxHPAttribute = FGameplayAttribute();

	Super::Deinitialize();
}

float UWxViewModel_Health::GetCurrentHP() const
{
	return CurrentHP;
}

void UWxViewModel_Health::SetCurrentHP(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentHP, NewValue);
}

float UWxViewModel_Health::GetMaxHP() const
{
	return MaxHP;
}

void UWxViewModel_Health::SetMaxHP(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxHP, NewValue);
}

float UWxViewModel_Health::GetHealthPercent() const
{
	return HealthPercent;
}

void UWxViewModel_Health::SetHealthPercent(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, NewValue);
}

void UWxViewModel_Health::HandleHPChanged(const FOnAttributeChangeData& Data)
{
	SetCurrentHP(Data.NewValue);
	RecalculateHealthPercent();
}

void UWxViewModel_Health::HandleMaxHPChanged(const FOnAttributeChangeData& Data)
{
	SetMaxHP(Data.NewValue);
	RecalculateHealthPercent();
}

void UWxViewModel_Health::RecalculateHealthPercent()
{
	const float Percent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
	SetHealthPercent(Percent);
}
