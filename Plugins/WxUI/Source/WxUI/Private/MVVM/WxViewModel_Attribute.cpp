// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"

void UWxViewModel_Attribute::Initialize(UAbilitySystemComponent* InASC, FGameplayAttribute InAttribute, FGameplayAttribute InMaxAttribute)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;

	if (InAttribute.IsValid())
	{
		BoundAttribute = InAttribute;
		SetCurrentAttribute(InASC->GetNumericAttribute(InAttribute));
		InASC->GetGameplayAttributeValueChangeDelegate(InAttribute)
			.AddUObject(this, &UWxViewModel_Attribute::HandleAttributeChanged);
	}

	if (InMaxAttribute.IsValid())
	{
		BoundMaxAttribute = InMaxAttribute;
		SetMaxAttribute(InASC->GetNumericAttribute(InMaxAttribute));
		InASC->GetGameplayAttributeValueChangeDelegate(InMaxAttribute)
			.AddUObject(this, &UWxViewModel_Attribute::HandleMaxAttributeChanged);
	}

	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (BoundAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BoundAttribute).RemoveAll(this);
		}

		if (BoundMaxAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BoundMaxAttribute).RemoveAll(this);
		}
	}

	CachedASC.Reset();
	BoundAttribute = FGameplayAttribute();
	BoundMaxAttribute = FGameplayAttribute();

	Super::Deinitialize();
}

float UWxViewModel_Attribute::GetCurrentAttribute() const
{
	return CurrentAttribute;
}

void UWxViewModel_Attribute::SetCurrentAttribute(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentAttribute, NewValue);
}

float UWxViewModel_Attribute::GetMaxAttribute() const
{
	return MaxAttribute;
}

void UWxViewModel_Attribute::SetMaxAttribute(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxAttribute, NewValue);
}

float UWxViewModel_Attribute::GetAttributePercent() const
{
	return AttributePercent;
}

void UWxViewModel_Attribute::SetAttributePercent(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(AttributePercent, NewValue);
}

void UWxViewModel_Attribute::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	SetCurrentAttribute(Data.NewValue);
	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::HandleMaxAttributeChanged(const FOnAttributeChangeData& Data)
{
	SetMaxAttribute(Data.NewValue);
	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::RecalculateAttributePercent()
{
	const float Percent = (MaxAttribute > 0.f) ? (CurrentAttribute / MaxAttribute) : 0.f;
	SetAttributePercent(Percent);
}
