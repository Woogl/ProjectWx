// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

#include "Engine/Texture2D.h"

void UWxViewModel_Character::Initialize(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();

	// 자식은 ASC 가 소유하는 공유본이다 — 같은 캐릭터를 보는 다른 위젯과 같은 인스턴스를 쓴다.
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, UWxViewModel_AbilitySystem::GetOrCreate(InASC));

	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, InCharacterName);
	RequestImageAsync(TEXT("Portrait"), InPortrait);
}

void UWxViewModel_Character::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	UE_MVVM_SET_PROPERTY_VALUE(Portrait, LoadedImage);
}

void UWxViewModel_Character::Deinitialize()
{
	// 자식은 공유본이라 해제하지 않고 놓기만 한다 — 죽이면 같은 인스턴스를 보고 있는 다른 위젯이 얼어붙는다.
	AbilitySystem = nullptr;

	CharacterName = FText::GetEmpty();
	Portrait = nullptr;

	Super::Deinitialize();
}
