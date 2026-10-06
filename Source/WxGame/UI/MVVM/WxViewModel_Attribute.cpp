// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"

void UWxViewModel_Attribute::Initialize(UAbilitySystemComponent* InASC, FGameplayAttribute InAttribute, FGameplayAttribute InMaxAttribute)
{
	Deinitialize();

	if (!InASC || !InAttribute.IsValid())
	{
		return;
	}

	InMaxAttribute = InMaxAttribute.IsValid() ? InMaxAttribute : InAttribute;

	BoundAttribute = InAttribute;
	CachedASC = InASC;
	BoundMaxAttribute = InMaxAttribute;

	UE_MVVM_SET_PROPERTY_VALUE(AttributeAmount, InASC->GetNumericAttribute(InAttribute));
	UE_MVVM_SET_PROPERTY_VALUE(MaxAttributeAmount, InASC->GetNumericAttribute(InMaxAttribute));
	RefreshDerivedFields();

	InASC->GetGameplayAttributeValueChangeDelegate(InAttribute)
		.AddUObject(this, &UWxViewModel_Attribute::HandleAttributeChanged);
	InASC->GetGameplayAttributeValueChangeDelegate(InMaxAttribute)
		.AddUObject(this, &UWxViewModel_Attribute::HandleMaxAttributeChanged);
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

	UE_MVVM_SET_PROPERTY_VALUE(AttributeAmount, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(MaxAttributeAmount, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(AttributePercent, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(IsAttributeFull, false);
}

FGameplayAttribute UWxViewModel_Attribute::GetBoundAttribute() const
{
	return BoundAttribute;
}

FGameplayAttribute UWxViewModel_Attribute::GetBoundMaxAttribute() const
{
	return BoundMaxAttribute;
}

void UWxViewModel_Attribute::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	UE_MVVM_SET_PROPERTY_VALUE(AttributeAmount, Data.NewValue);
	RefreshDerivedFields();
}

void UWxViewModel_Attribute::HandleMaxAttributeChanged(const FOnAttributeChangeData& Data)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxAttributeAmount, Data.NewValue);
	RefreshDerivedFields();
}

void UWxViewModel_Attribute::RefreshDerivedFields()
{
	UE_MVVM_SET_PROPERTY_VALUE(IsAttributeFull, AttributeAmount >= MaxAttributeAmount);
	UE_MVVM_SET_PROPERTY_VALUE(AttributePercent, MaxAttributeAmount > 0.f ? AttributeAmount / MaxAttributeAmount : 0.f);
}
