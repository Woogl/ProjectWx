// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Component/WxEffectComponent_UIData.h"
#include "MVVM/WxViewModel_Effect.h"

UWxViewModel_AbilitySystem* UWxViewModel_AbilitySystem::GetOrCreate(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return nullptr;
	}

	if (UWxViewModel* Existing = FindSharedViewModel(InASC, StaticClass()))
	{
		return CastChecked<UWxViewModel_AbilitySystem>(Existing);
	}

	UWxViewModel_AbilitySystem* ViewModel = NewObject<UWxViewModel_AbilitySystem>(InASC);
	ViewModel->Initialize(InASC);

	return ViewModel;
}

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

	BuildActiveEffectViewModels();
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
	
	// 자식은 배열에서 떼기만 한다 — 위젯이 아직 붙들고 있는 공유본을 끊으면 그 표시가 언다.
	// 여기는 BeginDestroy 에서만 도달하고, 그때 각 자식도 자기 BeginDestroy 로 구독·티커를 정리한다.
	CachedASC.Reset();
	AttributeViewModels.Empty();
	AbilityViewModels.Empty();
	ActiveEffectViewModels.Empty();
	OwnedTags.Reset();

	Super::Deinitialize();
}

UWxViewModel_Attribute* UWxViewModel_AbilitySystem::GetOrCreateAttributeViewModel(FGameplayAttribute Current, FGameplayAttribute Max)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !Current.IsValid())
	{
		return nullptr;
	}

	// 조회와 생성이 같은 값을 봐야 최대치 생략 요청과 명시 요청이 같은 것으로 판별된다.
	const FGameplayAttribute MaxAttribute = Max.IsValid() ? Max : Current;

	// 컨버전 함수는 소스 갱신마다 재실행될 수 있으므로, 이미 만든 VM 이 있으면 재사용한다.
	for (UWxViewModel_Attribute* Existing : AttributeViewModels)
	{
		if (Existing && Existing->GetBoundAttribute() == Current && Existing->GetBoundMaxAttribute() == MaxAttribute)
		{
			return Existing;
		}
	}

	UWxViewModel_Attribute* AttrVM = NewObject<UWxViewModel_Attribute>(this);
	AttrVM->Initialize(ASC, Current, MaxAttribute);
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

	// 요청 태그를 판별에 그대로 쓰면 포함 관계라, 넓은 질의가 먼저 만들어진 VM 을 잡아 생성 순서에 따라 결과가 갈린다.
	const UGameplayAbility* AbilityCDO = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasAll(InAbilityTags))
		{
			AbilityCDO = Spec.Ability;
			break;
		}
	}

	if (!AbilityCDO)
	{
		return nullptr;
	}

	for (UWxViewModel_Ability* Existing : AbilityViewModels)
	{
		if (Existing && Existing->GetBoundAbility() == AbilityCDO)
		{
			return Existing;
		}
	}

	UWxViewModel_Ability* AbilityVM = NewObject<UWxViewModel_Ability>(this);
	AbilityVM->Initialize(ASC, AbilityCDO);
	AbilityViewModels.Add(AbilityVM);
	return AbilityVM;
}

void UWxViewModel_AbilitySystem::BuildActiveEffectViewModels()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

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
			UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(this);
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

	UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(this);
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
