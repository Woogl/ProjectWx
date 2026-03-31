// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/WxEffectComponent_UIData.h"
#include "MVVM/WxViewModel_Effect.h"

void UWxViewModel_AbilitySystem::Initialize(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;
	
	CachedASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectAdded);
	CachedASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectRemoved);

	RebuildAbilityViewModels();
}

void UWxViewModel_AbilitySystem::RebuildAbilityViewModels()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	AbilityViewModels.Empty();

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UGameplayAbility* AbilityCDO = Spec.Ability;
		if (!AbilityCDO)
		{
			continue;
		}

		if (AbilityCDO->GetAssetTags().IsEmpty())
		{
			continue;
		}

		UWxViewModel_Ability* AbilityVM = NewObject<UWxViewModel_Ability>(ASC);
		AbilityVM->Initialize(ASC, AbilityCDO);
		AbilityViewModels.Add(AbilityVM);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AbilityViewModels);
}

void UWxViewModel_AbilitySystem::RebuildActiveEffectViewModels()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}
	
	ActiveEffectViewModels.Empty();
	
	// 활성화된 모든 GE 순회
	FGameplayEffectQuery Query;
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
		if (!Effect)
		{
			continue;
		}
		const UGameplayEffect* GE = Effect->Spec.Def;
		if (const UWxEffectComponent_UIData* UIData = GE->FindComponent<UWxEffectComponent_UIData>())
		{
			UTexture2D* EffectIcon = UIData->Icon.LoadSynchronous();
			FText EffectName = UIData->DisplayName;
			UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(ASC);
			EffectVM->Initialize(ASC, Handle, EffectName, EffectIcon);
			ActiveEffectViewModels.Add(EffectVM);
		}
	}
	
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
}

void UWxViewModel_AbilitySystem::Deinitialize()
{
	if (CachedASC.IsValid())
	{
		CachedASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		CachedASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
		CachedASC.Reset();
	}
	
	AbilityViewModels.Empty();
	ActiveEffectViewModels.Empty();

	Super::Deinitialize();
}

void UWxViewModel_AbilitySystem::HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	RebuildActiveEffectViewModels();
}

void UWxViewModel_AbilitySystem::HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect)
{
	RebuildActiveEffectViewModels();
}
