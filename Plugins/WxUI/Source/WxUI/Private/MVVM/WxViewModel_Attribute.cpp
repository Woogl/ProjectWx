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
	if (!InASC || !InAttribute.IsValid())
	{
		return;
	}

	InMaxAttribute = InMaxAttribute.IsValid() ? InMaxAttribute : InAttribute;

	BoundAttribute = InAttribute;
	BoundMaxAttribute = InMaxAttribute;

	UE_MVVM_SET_PROPERTY_VALUE(AttributeAmount, InASC->GetNumericAttribute(InAttribute));
	UE_MVVM_SET_PROPERTY_VALUE(MaxAttributeAmount, InASC->GetNumericAttribute(InMaxAttribute));
	RefreshDerivedFields();

	InASC->GetGameplayAttributeValueChangeDelegate(InAttribute)
		.AddUObject(this, &UWxViewModel_Attribute::HandleAttributeChanged);
	InASC->GetGameplayAttributeValueChangeDelegate(InMaxAttribute)
		.AddUObject(this, &UWxViewModel_Attribute::HandleMaxAttributeChanged);
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

UObject* UWxViewModelResolver_Attribute::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const IAbilitySystemInterface* AbilitySystemPawn = PC ? Cast<IAbilitySystemInterface>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = AbilitySystemPawn ? AbilitySystemPawn->GetAbilitySystemComponent() : nullptr;

	if (!ASC || !Attribute.IsValid())
	{
		return nullptr;
	}

	// ASC의 어빌리티시스템 VM이 어트리뷰트 조합별 인스턴스를 소유하므로 같은 조합을 보는 위젯이 공유한다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);

	return AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAttributeViewModel(Attribute, MaxAttribute) : nullptr;
}
