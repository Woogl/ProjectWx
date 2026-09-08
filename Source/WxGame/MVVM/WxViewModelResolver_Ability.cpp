// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Blueprint/UserWidget.h"
#include "MVVM/WxAbilitySlotSwitcher.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UAbilitySystemComponent* ASC = UWxAbilitySlotSwitcher::FindAbilitySystem(UserWidget);

	// 빈 컨테이너는 HasAll 이 항상 true 라 아무 어빌리티나 매칭된다.
	if (!ASC || AbilityTags.IsEmpty())
	{
		return nullptr;
	}

	// 슬롯 뷰모델의 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 슬롯을 보는 위젯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);
	if (!AbilitySystemViewModel)
	{
		return nullptr;
	}

	// 후보를 가르는 태그까지 담아야 후보마다 뷰모델이 하나씩 생긴다 — 슬롯 태그로만 조회하면 둘이 같은 것을 나눠 쓴다.
	// 아직 아무 후보도 부여되지 않았으면 슬롯 태그로 빈 뷰모델을 만들어 두고, 부여되는 순간 스위처가 갈아 끼운다.
	const UGameplayAbility* PickedAbility = UWxAbilitySlotSwitcher::PickAbility(*ASC, AbilityTags, nullptr);
	UWxViewModel_Ability* ViewModel = AbilitySystemViewModel->GetOrCreateAbilityViewModel(PickedAbility ? PickedAbility->GetAssetTags() : AbilityTags);
	if (!ViewModel)
	{
		return nullptr;
	}

	// 엔진이 리졸버에 const 위젯을 넘기지만, 상태를 따라 뷰모델을 갈아 끼우려면 위젯이 감시자를 소유해야 한다.
	UUserWidget* OwningWidget = const_cast<UUserWidget*>(UserWidget);
	UWxAbilitySlotSwitcher* Switcher = nullptr;
	for (UUserWidgetExtension* Extension : OwningWidget->GetExtensions(UWxAbilitySlotSwitcher::StaticClass()))
	{
		UWxAbilitySlotSwitcher* SlotSwitcher = Cast<UWxAbilitySlotSwitcher>(Extension);
		if (SlotSwitcher && SlotSwitcher->HandlesSlot(AbilityTags))
		{
			Switcher = SlotSwitcher;
			break;
		}
	}

	if (!Switcher)
	{
		Switcher = OwningWidget->AddExtension<UWxAbilitySlotSwitcher>();
	}

	if (Switcher)
	{
		Switcher->WatchSlot(ASC, AbilityTags, PickedAbility, ViewModel);
	}

	return ViewModel;
}
