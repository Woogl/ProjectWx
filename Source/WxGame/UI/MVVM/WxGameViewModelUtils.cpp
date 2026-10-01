// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxGameViewModelUtils.h"
#include "AbilitySystem/Effects/WxEffectComponent_UIData.h"
#include "Character/WxCharacterBase.h"
#include "GameplayEffect.h"
#include "UI/MVVM/WxViewModel_AbilitySystem.h"
#include "UI/MVVM/WxViewModel_Character.h"
#include "UI/MVVM/WxViewModel_Effect.h"

void WxGameViewModel::InitializeCharacter(UWxViewModel_Character& ViewModel, const AWxCharacterBase& Character)
{
	ViewModel.Initialize(Character.GetAbilitySystemComponent(), Character.GetTitle());
	if (UWxViewModel_AbilitySystem* AbilitySystem = ViewModel.AbilitySystem)
	{
		AbilitySystem->ConfigureEffectPresentation(FWxConfigureEffectViewModel::CreateLambda([](UWxViewModel_Effect& EffectViewModel, const UGameplayEffect& Effect)
		{
			const UWxEffectComponent_UIData* Data = Effect.FindComponent<UWxEffectComponent_UIData>();
			if (!Data || Data->GetIcon().IsNull())
			{
				return false;
			}

			EffectViewModel.SetPresentation(Data->GetTitle(), Data->GetDescription(), Data->GetIcon());
			return true;
		}));
	}
}
