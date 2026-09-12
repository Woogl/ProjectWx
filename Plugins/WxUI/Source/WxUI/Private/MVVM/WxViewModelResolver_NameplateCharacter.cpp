// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_NameplateCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_Nameplate.h"
#include "UObject/UObjectIterator.h"

UObject* UWxViewModelResolver_NameplateCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || UserWidget->IsDesignTime() || !ExpectedType)
	{
		return nullptr;
	}
	const bool bPresentation = ExpectedType == UWxViewModel_Nameplate::StaticClass();
	if (!bPresentation && !UWxViewModel_Character::StaticClass()->IsChildOf(ExpectedType))
	{
		return nullptr;
	}

	const UUserWidget* RootWidget = UserWidget;
	while (const UUserWidget* ParentWidget = RootWidget->GetTypedOuter<UUserWidget>())
	{
		RootWidget = ParentWidget;
	}

	// 엔진 위젯의 Outer는 대상 액터가 아니다. Resolver 실행 시에만 위젯을 보유한 컴포넌트를 역조회한다.
	UWidgetComponent* Nameplate = nullptr;
	for (TObjectIterator<UWidgetComponent> It; It; ++It)
	{
		if (IsValid(*It) && It->GetWorld() == RootWidget->GetWorld() && It->GetWidget() == RootWidget)
		{
			Nameplate = *It;
			break;
		}
	}
	if (bPresentation && Nameplate)
	{
		UWxViewModel_Nameplate* ViewModel = NewObject<UWxViewModel_Nameplate>(const_cast<UUserWidget*>(UserWidget));
		ViewModel->Initialize(Nameplate);
		return ViewModel;
	}
	UAbilitySystemComponent* ASC = Nameplate ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Nameplate->GetOwner()) : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	// 다른 화면이 쓰는 공유본을 재초기화하면 바인딩과 이미지 요청이 끊기므로 그대로 반환한다.
	if (UWxViewModel* Existing = UWxViewModel::FindSharedViewModel(ASC, UWxViewModel_Character::StaticClass()))
	{
		return Existing;
	}
	UWxViewModel_Character* ViewModel = UWxViewModel_Character::GetOrCreate(ASC);
	ViewModel->Initialize(ASC, Nameplate->GetOwner());
	return ViewModel;
}

void UWxViewModelResolver_NameplateCharacter::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	// 표시 갱신은 위젯별 수명이다. 같은 ASC를 보는 Character 공유본은 해제하지 않는다.
	if (UWxViewModel_Nameplate* NameplateViewModel = Cast<UWxViewModel_Nameplate>(ViewModel))
	{
		NameplateViewModel->Deinitialize();
	}
}
