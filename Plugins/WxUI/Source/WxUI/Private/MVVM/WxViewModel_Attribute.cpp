// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Attribute.h"
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
		SetCurrentAttribute(InitialValue);
		SetIsAttributeEmpty(InitialValue <= 0.f);
		SetIsAttributeFull(InMaxAttribute.IsValid() && InitialValue >= InASC->GetNumericAttribute(InMaxAttribute));
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
	SetCurrentAttribute(Data.NewValue);
	SetIsAttributeEmpty(Data.NewValue <= 0.f);
	SetIsAttributeFull(Data.NewValue >= MaxAttribute);
	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::HandleMaxAttributeChanged(const FOnAttributeChangeData& Data)
{
	SetMaxAttribute(Data.NewValue);
	SetIsAttributeFull(CurrentAttribute >= Data.NewValue);
	RecalculateAttributePercent();
}

void UWxViewModel_Attribute::RecalculateAttributePercent()
{
	const float Percent = (MaxAttribute > 0.f) ? (CurrentAttribute / MaxAttribute) : 0.f;
	SetAttributePercent(Percent);
}

UObject* UWxViewModelResolver_Attribute::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const IAbilitySystemInterface* AbilitySystemPawn = PC ? Cast<IAbilitySystemInterface>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = AbilitySystemPawn ? AbilitySystemPawn->GetAbilitySystemComponent() : nullptr;

	if (!ASC || !CurrentAttribute.IsValid())
	{
		return nullptr;
	}

	// 위젯이 아닌 데이터 소스(ASC)를 Outer 로 생성한다.
	// 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_Attribute* ViewModel = NewObject<UWxViewModel_Attribute>(ASC);
	ViewModel->Initialize(ASC, CurrentAttribute, MaxAttribute.IsValid() ? MaxAttribute : CurrentAttribute);

	return ViewModel;
}
