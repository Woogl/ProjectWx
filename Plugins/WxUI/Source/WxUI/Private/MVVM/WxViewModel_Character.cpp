// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "UObject/UObjectHash.h"

UWxViewModel_Character* UWxViewModel_Character::GetOrCreate(UWxViewModel_AbilitySystem* InAbilitySystem, const FText& InCharacterName)
{
	if (!IsValid(InAbilitySystem))
	{
		return nullptr;
	}

	// 데이터 소스를 Outer 로 만들어 두므로, 소스의 자식 중에서 찾으면 공유본이다.
	if (UWxViewModel_Character* Existing = static_cast<UWxViewModel_Character*>(FindObjectWithOuter(InAbilitySystem, StaticClass())))
	{
		return Existing;
	}

	UWxViewModel_Character* ViewModel = NewObject<UWxViewModel_Character>(InAbilitySystem);
	ViewModel->Initialize(InAbilitySystem, InCharacterName);
	return ViewModel;
}

void UWxViewModel_Character::Initialize(UWxViewModel_AbilitySystem* InAbilitySystem, FText InCharacterName)
{
	Deinitialize();

	if (!InAbilitySystem)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, InAbilitySystem);
	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, InCharacterName);
}

void UWxViewModel_Character::Deinitialize()
{
	// 자식은 공유본이라 해제하지 않고 놓기만 한다 — 죽이면 같은 인스턴스를 보고 있는 다른 위젯이 얼어붙는다.
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, nullptr);
	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, FText::GetEmpty());
}
