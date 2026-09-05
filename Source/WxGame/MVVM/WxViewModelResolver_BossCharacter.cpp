// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_BossCharacter.h"

#include "Blueprint/UserWidget.h"
#include "MVVM/WxViewModel_BossDisplay.h"

UObject* UWxViewModelResolver_BossCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWorld* World = UserWidget ? UserWidget->GetWorld() : nullptr;
	if (!World || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_BossDisplay::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	UWxViewModel_BossDisplay* ViewModel = NewObject<UWxViewModel_BossDisplay>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
	ViewModel->StartObserving(World);
	return ViewModel;
}

void UWxViewModelResolver_BossCharacter::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_BossDisplay* BossDisplay = Cast<UWxViewModel_BossDisplay>(ViewModel))
	{
		BossDisplay->Deinitialize();
	}
}
