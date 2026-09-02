// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectUIData.h"
#include "MVVM/WxViewModel_Effect.h"
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

	InASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectAdded);
	InASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxViewModel_AbilitySystem::HandleActiveEffectRemoved);
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_AbilitySystem::HandleTagChanged);
	InASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UWxViewModel_AbilitySystem::HandleAbilitySpecDirtied);

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
		ASC->AbilitySpecDirtiedCallbacks.RemoveAll(this);
	}

	if (OwnedTagsRefreshHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(OwnedTagsRefreshHandle);
		OwnedTagsRefreshHandle.Reset();
	}

	if (AbilityRebindHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(AbilityRebindHandle);
		AbilityRebindHandle.Reset();
	}

	// 자식은 배열에서 떼기만 한다 — 위젯이 아직 붙들고 있는 공유본을 끊으면 그 표시가 언다.
	// 자식이 이 VM 을 Outer 로 삼아 살려 두므로, 파괴로 여기 닿았다면 자식을 붙든 위젯도 없고 각 자식은 자기 BeginDestroy 로 구독·티커를 정리한다.
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

	// 이미 활성인 GE 는 추가 통지가 다시 오지 않으므로, 지금 목록으로 그 통지를 대신 태운다.
	FGameplayEffectQuery Query;
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle))
		{
			HandleActiveEffectAdded(ASC, Effect->Spec, Handle);
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
	if (!Spec.Def)
	{
		return;
	}

	// GE 의 컴포넌트 배열은 클래스로만 뒤질 수 있어, 도메인 구현체와 공유하는 엔진 베이스를 앵커로 잡고 계약으로 내린다.
	const IWxUIData* UIData = Cast<IWxUIData>(Spec.Def->FindComponent<UGameplayEffectUIData>());

	// 수치만 쓰는 GE 도 같은 앵커에 걸리므로, 아이콘을 채운 GE 만 목록에 올린다 — 버프 목록은 아이콘으로 그려진다.
	if (!UIData || UIData->GetIcon().IsNull())
	{
		return;
	}

	UWxViewModel_Effect* EffectVM = NewObject<UWxViewModel_Effect>(this);
	EffectVM->Initialize(InASC, Handle, UIData);

	// 초기화가 핸들을 잡지 못했으면 제거 통지와 영영 매칭되지 않아 목록에 유령으로 남는다.
	if (!EffectVM->GetBoundHandle().IsValid())
	{
		return;
	}

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
	// 통지는 바뀐 태그의 부모까지 오고 GE 하나가 태그를 여럿 부여하므로, 한 프레임에 열댓 번이 몰려도 결과는 마지막 한 번과 같다.
	if (OwnedTagsRefreshHandle.IsValid())
	{
		return;
	}

	OwnedTagsRefreshHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWxViewModel_AbilitySystem::FlushOwnedTagsRefresh)
	);
}

void UWxViewModel_AbilitySystem::HandleAbilitySpecDirtied(const FGameplayAbilitySpec& Spec)
{
	// 스펙은 발동·종료로도 더러워지고 세트 부여는 한 프레임에 열댓 번이 몰리므로, 슬롯 재매칭은 프레임당 한 번으로 모은다.
	if (AbilityRebindHandle.IsValid())
	{
		return;
	}

	AbilityRebindHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWxViewModel_AbilitySystem::FlushAbilityRebind)
	);
}

bool UWxViewModel_AbilitySystem::FlushOwnedTagsRefresh(float DeltaTime)
{
	OwnedTagsRefreshHandle.Reset();
	RefreshOwnedTags();

	return false;
}

bool UWxViewModel_AbilitySystem::FlushAbilityRebind(float DeltaTime)
{
	AbilityRebindHandle.Reset();

	// 교체는 제거 뒤 부여라, 마지막에 오는 부여 신호 하나로 전부를 훑어야 비게 된 슬롯까지 같이 정리된다.
	for (UWxViewModel_Ability* AbilityVM : AbilityViewModels)
	{
		if (AbilityVM)
		{
			AbilityVM->RefreshBoundAbility();
		}
	}

	return false;
}
