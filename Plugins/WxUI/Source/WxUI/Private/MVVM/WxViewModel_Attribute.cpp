// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Attribute.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

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
		const float InitialValue = InASC->GetNumericAttribute(InAttribute);
		SetAttributeAmount(InitialValue);
		SetIsAttributeEmpty(InitialValue <= 0.f);
		SetIsAttributeFull(InMaxAttribute.IsValid() && InitialValue >= InASC->GetNumericAttribute(InMaxAttribute));
		InASC->GetGameplayAttributeValueChangeDelegate(InAttribute)
			.AddUObject(this, &UWxViewModel_Attribute::HandleAttributeChanged);
	}

	if (InMaxAttribute.IsValid())
	{
		BoundMaxAttribute = InMaxAttribute;
		SetMaxAttributeAmount(InASC->GetNumericAttribute(InMaxAttribute));
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

float UWxViewModel_Attribute::GetAttributeAmount() const
{
	return AttributeAmount;
}

void UWxViewModel_Attribute::SetAttributeAmount(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(AttributeAmount, NewValue);
}

float UWxViewModel_Attribute::GetMaxAttributeAmount() const
{
	return MaxAttributeAmount;
}

void UWxViewModel_Attribute::SetMaxAttributeAmount(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxAttributeAmount, NewValue);
}

float UWxViewModel_Attribute::GetAttributePercent() const
{
	return AttributePercent;
}

void UWxViewModel_Attribute::SetAttributePercent(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(AttributePercent, NewValue);
}

bool UWxViewModel_Attribute::GetIsAttributeEmpty() const
{
	return IsAttributeEmpty;
}

void UWxViewModel_Attribute::SetIsAttributeEmpty(bool bNewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(IsAttributeEmpty, bNewValue);
}

bool UWxViewModel_Attribute::GetIsAttributeFull() const
{
	return IsAttributeFull;
}

void UWxViewModel_Attribute::SetIsAttributeFull(bool bNewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(IsAttributeFull, bNewValue);
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
	SetAttributeAmount(Data.NewValue);
	SetIsAttributeEmpty(Data.NewValue <= 0.f);
	SetIsAttributeFull(Data.NewValue >= MaxAttributeAmount);
	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::HandleMaxAttributeChanged(const FOnAttributeChangeData& Data)
{
	SetMaxAttributeAmount(Data.NewValue);
	SetIsAttributeFull(AttributeAmount >= Data.NewValue);
	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::RecalculateAttributePercent()
{
	const float Percent = (MaxAttributeAmount > 0.f) ? (AttributeAmount / MaxAttributeAmount) : 0.f;
	SetAttributePercent(Percent);
}

UObject* UWxViewModelResolver_Attribute::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const IAbilitySystemInterface* AbilitySystemPawn = PC ? Cast<IAbilitySystemInterface>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = AbilitySystemPawn ? AbilitySystemPawn->GetAbilitySystemComponent() : nullptr;

	if (!ASC || !Attribute.IsValid())
	{
		return nullptr;
	}

	// 인스턴스는 ASC 의 어빌리티시스템 VM 이 어트리뷰트 단위로 소유한다 — 같은 어트리뷰트를 보는 위젯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);

	return AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAttributeViewModel(Attribute, MaxAttribute) : nullptr;
}
