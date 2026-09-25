// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_AbilitySystem.h"
#include "AbilitySystem/Effect/WxEffectComponent_UIData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Effect.h"

namespace
{
	bool ConfigureEffectPresentation(UWxViewModel_Effect& ViewModel, const UGameplayEffect& Effect)
	{
		const UWxEffectComponent_UIData* Data = Effect.FindComponent<UWxEffectComponent_UIData>();
		if (!Data || Data->GetIcon().IsNull())
		{
			return false;
		}

		ViewModel.SetPresentation(Data->GetTitle(), Data->GetDescription(), Data->GetIcon());
		return true;
	}
}

UObject* UWxViewModelResolver_AbilitySystem::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	return GetOrCreate(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC ? PC->GetPawn() : nullptr));
}

UWxViewModel_AbilitySystem* UWxViewModelResolver_AbilitySystem::GetOrCreate(UAbilitySystemComponent* InASC)
{
	UWxViewModel_AbilitySystem* ViewModel = UWxViewModel_AbilitySystem::GetOrCreate(InASC);
	if (ViewModel)
	{
		ViewModel->ConfigureEffectPresentation(FWxConfigureEffectViewModel::CreateStatic(&ConfigureEffectPresentation));
	}
	return ViewModel;
}
