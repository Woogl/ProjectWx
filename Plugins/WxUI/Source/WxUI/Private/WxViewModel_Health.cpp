// Copyright Woogle. All Rights Reserved.

#include "WxViewModel_Health.h"
#include "AbilitySystemComponent.h"

namespace
{
	FGameplayAttribute FindAttributeOnASC(const UAbilitySystemComponent* ASC, FName AttributeName)
	{
		for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
		{
			if (FProperty* Prop = Set->GetClass()->FindPropertyByName(AttributeName))
			{
				return FGameplayAttribute(Prop);
			}
		}
		return FGameplayAttribute();
	}
}

void UWxViewModel_Health::InitializeWithASC(UAbilitySystemComponent* InASC, FName InHPName, FName InMaxHPName)
{
	if (!InASC)
	{
		return;
	}

	DeinitializeFromASC();
	CachedASC = InASC;
	HPName = InHPName;
	MaxHPName = InMaxHPName;

	const FGameplayAttribute HPAttr = FindAttributeOnASC(InASC, HPName);
	const FGameplayAttribute MaxHPAttr = FindAttributeOnASC(InASC, MaxHPName);

	if (HPAttr.IsValid())
	{
		SetCurrentHP(InASC->GetNumericAttribute(HPAttr));
		InASC->GetGameplayAttributeValueChangeDelegate(HPAttr)
			.AddUObject(this, &UWxViewModel_Health::HandleHPChanged);
	}

	if (MaxHPAttr.IsValid())
	{
		SetMaxHP(InASC->GetNumericAttribute(MaxHPAttr));
		InASC->GetGameplayAttributeValueChangeDelegate(MaxHPAttr)
			.AddUObject(this, &UWxViewModel_Health::HandleMaxHPChanged);
	}

	RecalculateHealthPercent();
}

void UWxViewModel_Health::Deinitialize()
{
	DeinitializeFromASC();
}

void UWxViewModel_Health::DeinitializeFromASC()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		const FGameplayAttribute HPAttr = FindAttributeOnASC(ASC, HPName);
		const FGameplayAttribute MaxHPAttr = FindAttributeOnASC(ASC, MaxHPName);

		if (HPAttr.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(HPAttr).RemoveAll(this);
		}

		if (MaxHPAttr.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(MaxHPAttr).RemoveAll(this);
		}
	}

	CachedASC.Reset();
	HPName = NAME_None;
	MaxHPName = NAME_None;
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
