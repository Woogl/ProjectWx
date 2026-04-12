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

	InitializeAttributeViewModels();
	InitializeAbilityViewModels();
	RefreshActiveEffectViewModels();
	RefreshOwnedTags();
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

UWxViewModel_Ability* UWxViewModel_AbilitySystem::FindAbilityViewModel(FGameplayTag InAbilityTag) const
{
	for (UWxViewModel_Ability* VM : AbilityViewModels)
	{
		if (VM && VM->AbilityTag == InAbilityTag)
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

void UWxViewModel_AbilitySystem::InitializeAttributeViewModels()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	AttributeViewModels.Empty();

	TArray<FGameplayAttribute> AllAttributes;
	ASC->GetAllAttributes(AllAttributes);

	// 어트리뷰트 이름으로 맵 구축
	TMap<FString, FGameplayAttribute> AttributeMap;
	for (const FGameplayAttribute& Attr : AllAttributes)
	{
		AttributeMap.Add(Attr.GetName(), Attr);
	}

	// Max 대응 어트리뷰트를 탐색하여 뷰모델 생성
	// Max 대응 어트리뷰트가 없으면 자기 자신을 Max로 뷰모델 생성
	for (const auto& Pair : AttributeMap)
	{
		if (Pair.Key.StartsWith(TEXT("Max")))
		{
			continue;
		}

		FString MaxName = TEXT("Max") + Pair.Key;
		const FGameplayAttribute* MaxAttr = AttributeMap.Find(MaxName);

		UWxViewModel_Attribute* AttrVM = NewObject<UWxViewModel_Attribute>(ASC);
		AttrVM->Initialize(ASC, Pair.Value, MaxAttr ? *MaxAttr : Pair.Value);
		AttributeViewModels.Add(AttrVM);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AttributeViewModels);
}

void UWxViewModel_AbilitySystem::InitializeAbilityViewModels()
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

void UWxViewModel_AbilitySystem::RefreshActiveEffectViewModels()
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
			UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(ASC);
			EffectVM->Initialize(ASC, Handle, UIData);
			ActiveEffectViewModels.Add(EffectVM);
		}
	}
	
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
}

void UWxViewModel_AbilitySystem::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}
	CachedASC.Reset();

	AttributeViewModels.Empty();
	AbilityViewModels.Empty();
	ActiveEffectViewModels.Empty();
	OwnedTags.Reset();

	Super::Deinitialize();
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
		if (ActiveEffectViewModels[i] && ActiveEffectViewModels[i]->GetBoundHandle() == ActiveEffect.Handle)
		{
			ActiveEffectViewModels.RemoveAt(i);
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
			break;
		}
	}
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

void UWxViewModel_AbilitySystem::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshOwnedTags();
}
