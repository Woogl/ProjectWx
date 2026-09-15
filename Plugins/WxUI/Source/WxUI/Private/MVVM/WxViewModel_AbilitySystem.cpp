// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffectUIData.h"
#include "MVVM/WxViewModel_Effect.h"
#include "TimerManager.h"
#include "WxUIData.h"

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

	CachedASC = InASC;

	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_AbilitySystem::HandleTagChanged);
	InASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UWxViewModel_AbilitySystem::HandleAbilitySpecDirtied);

	RefreshOwnedTags();
}

const TArray<TObjectPtr<UWxViewModel_Effect>>& UWxViewModel_AbilitySystem::GetActiveEffectViewModels() const
{
	// 리플렉션 Getter는 const 계약이므로, 표시 데이터의 지연 초기화만 비const 경로로 넘긴다.
	const_cast<UWxViewModel_AbilitySystem*>(this)->InitializeActiveEffects();
	return ActiveEffectViewModels;
}

void UWxViewModel_AbilitySystem::InitializeActiveEffects()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();

	// 추가 통지를 구독했으면 목록도 이미 구성돼 있다.
	if (!ASC || ASC->OnActiveGameplayEffectAddedDelegateToSelf.IsBoundToObject(this))
	{
		return;
	}

	ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectAdded);
	ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectRemoved);
	BuildActiveEffectViewModels();
}

void UWxViewModel_AbilitySystem::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
		ASC->AbilitySpecDirtiedCallbacks.RemoveAll(this);

		if (UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(OwnedTagsRefreshHandle);
			World->GetTimerManager().ClearTimer(AbilityRebindHandle);
		}
	}

	// 자식은 배열에서 떼기만 한다 — 위젯이 아직 붙들고 있는 공유본을 끊으면 그 표시가 언다.
	// 자식이 이 VM 을 Outer 로 삼아 살려 두므로, 파괴로 여기 닿았다면 자식을 붙든 위젯도 없고 각 자식은 자기 BeginDestroy 로 구독·티커를 정리한다.
	CachedASC.Reset();
	AttributeViewModels.Empty();
	AbilityViewModels.Empty();
	ActiveEffectViewModels.Empty();
	OwnedTags.Reset();
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(OwnedTags);
	}

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

void UWxViewModel_AbilitySystem::BuildActiveEffectViewModels()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	// Getter 안에서는 같은 필드의 변경을 통지하지 않고 현재 스냅샷만 구성한다.
	FGameplayEffectQuery Query;
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle))
		{
			AddActiveEffectViewModel(ASC, Effect->Spec, Handle);
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

void UWxViewModel_AbilitySystem::HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (AddActiveEffectViewModel(InASC, Spec, Handle))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels);
	}
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

	// GE 의 컴포넌트 배열은 클래스로만 뒤질 수 있어, 도메인 구현체와 공유하는 엔진 베이스를 앵커로 잡고 계약으로 내린다.
	const IWxUIData* UIData = Cast<IWxUIData>(Spec.Def->FindComponent<UGameplayEffectUIData>());

	// 수치만 쓰는 GE 도 같은 앵커에 걸리므로, 아이콘을 채운 GE 만 목록에 올린다 — 버프 목록은 아이콘으로 그려진다.
	if (!UIData || UIData->GetIcon().IsNull())
	{
		return false;
	}

	UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(this);
	EffectVM->Initialize(InASC, Handle, UIData);

	// 초기화가 핸들을 잡지 못했으면 제거 통지와 영영 매칭되지 않아 목록에 유령으로 남는다.
	if (!EffectVM->GetBoundHandle().IsValid())
	{
		return false;
	}

	ActiveEffectViewModels.Add(EffectVM);
	return true;
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

void UWxViewModel_AbilitySystem::HandleAbilitySpecDirtied(const FGameplayAbilitySpec& Spec)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;

	// 스펙은 발동·종료로도 더러워지고 세트 부여는 한 프레임에 열댓 번이 몰리므로, 슬롯 재매칭은 프레임당 한 번으로 모은다.
	if (!World || World->GetTimerManager().IsTimerActive(AbilityRebindHandle))
	{
		return;
	}

	AbilityRebindHandle = World->GetTimerManager().SetTimerForNextTick(this, &UWxViewModel_AbilitySystem::FlushAbilityRebind);
}

void UWxViewModel_AbilitySystem::FlushOwnedTagsRefresh()
{
	OwnedTagsRefreshHandle.Invalidate();
	RefreshOwnedTags();
}

void UWxViewModel_AbilitySystem::FlushAbilityRebind()
{
	AbilityRebindHandle.Invalidate();

	// 교체는 제거 뒤 부여라, 마지막에 오는 부여 신호 하나로 전부를 훑어야 비게 된 슬롯까지 같이 정리된다.
	for (UWxViewModel_Ability* AbilityVM : AbilityViewModels)
	{
		if (AbilityVM)
		{
			AbilityVM->RefreshBoundAbility();
		}
	}
}
