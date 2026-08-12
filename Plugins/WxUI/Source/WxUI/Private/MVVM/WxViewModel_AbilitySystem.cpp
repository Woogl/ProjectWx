// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Component/WxEffectComponent_UIData.h"
#include "MVVM/WxViewModel_Effect.h"

void UWxViewModel_AbilitySystem::Initialize(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;
	
	InASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectAdded);
	InASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectRemoved);
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_AbilitySystem::HandleTagChanged);

	RefreshActiveEffectViewModels();
	RefreshOwnedTags();
}

void UWxViewModel_AbilitySystem::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}
	
	// 자식 VM 의 티커·ASC 구독을 즉시 정리한다.
	// Empty() 로 배열에서만 떼면 자식은 GC 의 BeginDestroy→Deinitialize 까지 티킹·구독을 유지한다(Character VM 패턴과 동일).
	for (UWxViewModel_Attribute* AttributeVM : AttributeViewModels)
	{
		if (AttributeVM)
		{
			AttributeVM->Deinitialize();
		}
	}
	for (UWxViewModel_Ability* AbilityVM : AbilityViewModels)
	{
		if (AbilityVM)
		{
			AbilityVM->Deinitialize();
		}
	}
	ClearActiveEffectViewModels();

	CachedASC.Reset();
	AttributeViewModels.Empty();
	AbilityViewModels.Empty();
	OwnedTags.Reset();

	Super::Deinitialize();
}

void UWxViewModel_AbilitySystem::ClearActiveEffectViewModels()
{
	for (UWxViewModel_Effect* EffectVM : ActiveEffectViewModels)
	{
		if (EffectVM)
		{
			EffectVM->Deinitialize();
		}
	}
	ActiveEffectViewModels.Empty();
}

UWxViewModel_Attribute* UWxViewModel_AbilitySystem::FindAttributeViewModel(FGameplayAttribute InAttribute) const
{
	for (UWxViewModel_Attribute* VM : AttributeViewModels)
	{
		if (VM && VM->GetBoundAttribute() == InAttribute)
		{
			return VM;
		}
	}
	return nullptr;
}

UWxViewModel_Ability* UWxViewModel_AbilitySystem::FindAbilityViewModel(const FGameplayTagContainer& InAbilityTags) const
{
	for (UWxViewModel_Ability* VM : AbilityViewModels)
	{
		if (VM && VM->AbilityTags.HasAll(InAbilityTags))
		{
			return VM;
		}
	}
	return nullptr;
}

UWxViewModel_Effect* UWxViewModel_AbilitySystem::FindActiveEffectViewModel(FGameplayTag InEffectTag) const
{
	for (UWxViewModel_Effect* VM : ActiveEffectViewModels)
	{
		if (VM && VM->EffectTag == InEffectTag)
		{
			return VM;
		}
	}
	return nullptr;
}

UWxViewModel_Attribute* UWxViewModel_AbilitySystem::GetOrCreateAttributeViewModel(FGameplayAttribute Current, FGameplayAttribute Max)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !Current.IsValid())
	{
		return nullptr;
	}

	// 컨버전 함수는 소스 갱신마다 재실행될 수 있으므로, 이미 만든 VM 이 있으면 재사용한다.
	if (UWxViewModel_Attribute* Existing = FindAttributeViewModel(Current))
	{
		return Existing;
	}

	UWxViewModel_Attribute* AttrVM = NewObject<UWxViewModel_Attribute>(ASC);
	AttrVM->Initialize(ASC, Current, Max.IsValid() ? Max : Current);
	AttributeViewModels.Add(AttrVM);
	return AttrVM;
}

UWxViewModel_Ability* UWxViewModel_AbilitySystem::GetOrCreateAbilityViewModel(const FGameplayTagContainer& InAbilityTags)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();

	// 빈 컨테이너는 HasAll 이 항상 true 라 아무 어빌리티나 매칭되므로 거부한다.
	if (!ASC || InAbilityTags.IsEmpty())
	{
		return nullptr;
	}

	// 컨버전 함수는 소스 갱신마다 재실행될 수 있으므로, 이미 만든 VM 이 있으면 재사용한다.
	if (UWxViewModel_Ability* Existing = FindAbilityViewModel(InAbilityTags))
	{
		return Existing;
	}

	// FindAbilityViewModel 과 동일한 HasAll 술어로 매칭해야 재평가 시 중복 생성되지 않는다.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UGameplayAbility* AbilityCDO = Spec.Ability;
		if (!AbilityCDO || !AbilityCDO->GetAssetTags().HasAll(InAbilityTags))
		{
			continue;
		}

		UWxViewModel_Ability* AbilityVM = NewObject<UWxViewModel_Ability>(ASC);
		AbilityVM->Initialize(ASC, AbilityCDO);
		AbilityViewModels.Add(AbilityVM);
		return AbilityVM;
	}
	return nullptr;
}

void UWxViewModel_AbilitySystem::RefreshActiveEffectViewModels()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}
	
	ClearActiveEffectViewModels();

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
		if (!GE)
		{
			continue;
		}
		if (const UWxEffectComponent_UIData* UIData = GE->FindComponent<UWxEffectComponent_UIData>())
		{
			UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(ASC);
			EffectVM->Initialize(ASC, Handle, UIData);
			ActiveEffectViewModels.Add(EffectVM);
		}
	}
	
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
}

void UWxViewModel_AbilitySystem::RefreshOwnedTags()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer NewTags;
	ASC->GetOwnedGameplayTags(NewTags);

	if (OwnedTags != NewTags)
	{
		OwnedTags = NewTags;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(OwnedTags);
	}
}

void UWxViewModel_AbilitySystem::HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!Spec.Def)
	{
		return;
	}

	const UWxEffectComponent_UIData* UIData = Spec.Def->FindComponent<UWxEffectComponent_UIData>();
	if (!UIData)
	{
		return;
	}

	UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(InASC);
	EffectVM->Initialize(InASC, Handle, UIData);
	ActiveEffectViewModels.Add(EffectVM);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
}

void UWxViewModel_AbilitySystem::HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect)
{
	for (int32 i = 0; i < ActiveEffectViewModels.Num(); ++i)
	{
		UWxViewModel_Effect* EffectVM = ActiveEffectViewModels[i];
		if (EffectVM && EffectVM->GetBoundHandle() == ActiveEffect.Handle)
		{
			EffectVM->Deinitialize();
			ActiveEffectViewModels.RemoveAt(i);
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
			break;
		}
	}
}

void UWxViewModel_AbilitySystem::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshOwnedTags();
}
