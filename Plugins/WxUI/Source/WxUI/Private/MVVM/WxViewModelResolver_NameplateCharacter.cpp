// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_NameplateCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Component/WxNameplateComponent.h"
#include "MVVM/WxViewModel_Character.h"
#include "UObject/UObjectIterator.h"
#include "WxUIModule.h"

UObject* UWxViewModelResolver_NameplateCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || UserWidget->IsDesignTime() || !ExpectedType || !UWxViewModel_Character::StaticClass()->IsChildOf(ExpectedType))
	{
		return nullptr;
	}

	// 엔진 위젯의 Outer는 대상 액터가 아니다. MVVM 생성 시에만 위젯을 보유한 컴포넌트를 찾는다.
	for (TObjectIterator<UWxNameplateComponent> It; It; ++It)
	{
		if (IsValid(*It) && It->GetWidget() == UserWidget)
		{
			AActor* Owner = It->GetOwner();
			UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
			if (!ASC)
			{
				UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 위젯 구성 전에 Owner의 ASC가 필요하다. Owner=%s"), *GetNameSafe(Owner));
			}
			return UWxViewModel_Character::GetOrCreate(ASC, Owner);
		}
	}
	UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 위젯을 보유한 NameplateComponent가 없다. Widget=%s"), *GetNameSafe(UserWidget));
	return nullptr;
}
