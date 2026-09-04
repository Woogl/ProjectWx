// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

#include "Engine/Texture2D.h"

UWxViewModel_Character* UWxViewModel_Character::GetOrCreate(UObject* Source)
{
	if (!Source)
	{
		return nullptr;
	}

	if (UWxViewModel* Existing = FindSharedViewModel(Source, StaticClass()))
	{
		return CastChecked<UWxViewModel_Character>(Existing);
	}

	return NewObject<UWxViewModel_Character>(Source);
}

void UWxViewModel_Character::Initialize(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();

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

	// 파괴 경로에서는 통지하지 않는다 — 함께 수거될 뷰의 바인딩으로 통지가 들어간다.
	// 그 밖의 해제는 소스가 빠졌다는 사실 자체가 표시돼야 하므로 통지한다.
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AbilitySystem);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CharacterName);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Portrait);
	}

	Super::Deinitialize();
}
