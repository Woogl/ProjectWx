// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_AbilitySystem.h"
#include "UI/MVVM/WxViewModel_Ability.h"
#include "UI/MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "UI/MVVM/WxViewModel_Effect.h"
#include "TimerManager.h"

void UWxViewModel_AbilitySystem::Initialize(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	CachedASC = InASC;

	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_AbilitySystem::HandleTagChanged);
	InASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectAdded);
	InASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectRemoved);

	RefreshOwnedTags();
	for (const FActiveGameplayEffectHandle& Handle : InASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		if (const FActiveGameplayEffect* Effect = InASC->GetActiveGameplayEffect(Handle))
		{
			AddActiveEffectViewModel(InASC, Effect->Spec, Handle);
		}
	}
}

void UWxViewModel_AbilitySystem::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
		if (UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(OwnedTagsRefreshHandle);
		}
	}
	OwnedTagsRefreshHandle.Invalidate();
	// 아래 변경 알림으로 다시 도는 변환 함수가 자식 VM 을 새로 만들지 않게 먼저 놓는다.
	CachedASC.Reset();

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
	for (UWxViewModel_Effect* EffectVM : ActiveEffectViewModels)
	{
		if (EffectVM)
		{
			EffectVM->Deinitialize();
		}
	}
	AttributeViewModels.Reset();
	AbilityViewModels.Reset();
	if (!ActiveEffectViewModels.IsEmpty())
	{
		ActiveEffectViewModels.Reset();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
	}
	if (!OwnedTags.IsEmpty())
	{
		OwnedTags.Reset();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(OwnedTags);
	}
}

UAbilitySystemComponent* UWxViewModel_AbilitySystem::GetBoundASC() const
{
	return CachedASC.Get();
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

	for (UWxViewModel_Ability* Existing : AbilityViewModels)
	{
		if (Existing && Existing->GetAbilityTags() == InAbilityTags)
		{
			return Existing;
		}
	}

	UWxViewModel_Ability* AbilityVM = NewObject<UWxViewModel_Ability>(this);
	AbilityVM->Initialize(ASC, InAbilityTags);
	AbilityViewModels.Add(AbilityVM);
	return AbilityVM;
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
	if (AddActiveEffectViewModel(InASC, Spec, Handle))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
	}
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
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;

	// 통지는 바뀐 태그의 부모까지 오고 GE 하나가 태그를 여럿 부여하므로, 한 프레임에 열댓 번이 몰려도 결과는 마지막 한 번과 같다.
	if (!World || World->GetTimerManager().IsTimerActive(OwnedTagsRefreshHandle))
	{
		return;
	}

	OwnedTagsRefreshHandle = World->GetTimerManager().SetTimerForNextTick(this, &UWxViewModel_AbilitySystem::FlushOwnedTagsRefresh);
}

void UWxViewModel_AbilitySystem::FlushOwnedTagsRefresh()
{
	OwnedTagsRefreshHandle.Invalidate();
	RefreshOwnedTags();
}

bool UWxViewModel_AbilitySystem::AddActiveEffectViewModel(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!Handle.IsValid() || !Spec.Def)
	{
		return false;
	}

	// 억제 해제도 같은 핸들로 추가 통지를 보내므로 기존 VM과 스택 구독을 유지한다.
	for (const UWxViewModel_Effect* Existing : ActiveEffectViewModels)
	{
		if (Existing && Existing->GetBoundHandle() == Handle)
		{
			return false;
		}
	}

	UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(this);
	EffectVM->Initialize(InASC, Handle);

	// 초기화가 핸들을 잡지 못했으면 제거 통지와 영영 매칭되지 않아 목록에 유령으로 남는다.
	if (!EffectVM->GetBoundHandle().IsValid())
	{
		return false;
	}

	ActiveEffectViewModels.Add(EffectVM);
	return true;
}
