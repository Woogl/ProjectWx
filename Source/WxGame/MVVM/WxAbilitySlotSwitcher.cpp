// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxAbilitySlotSwitcher.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "View/MVVMView.h"
#include "View/MVVMViewClass.h"

const UGameplayAbility* UWxAbilitySlotSwitcher::PickAbility(const UAbilitySystemComponent& ASC, const FGameplayTagContainer& SlotTags, const UGameplayAbility* PreviousAbility)
{
	const UGameplayAbility* FallbackAbility = nullptr;

	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.Ability->GetAssetTags().HasAll(SlotTags))
		{
			continue;
		}

		if (Spec.Ability->DoesAbilitySatisfyTagRequirements(ASC))
		{
			return Spec.Ability;
		}

		// 사망처럼 후보가 전부 막히는 구간에는 보던 얼굴을 유지한다.
		if (!FallbackAbility || Spec.Ability.Get() == PreviousAbility)
		{
			FallbackAbility = Spec.Ability;
		}
	}

	return FallbackAbility;
}

UAbilitySystemComponent* UWxAbilitySlotSwitcher::FindAbilitySystem(const UUserWidget* Widget)
{
	const APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr;
	const IAbilitySystemInterface* AbilitySystemPawn = PC ? Cast<IAbilitySystemInterface>(PC->GetPawn()) : nullptr;

	return AbilitySystemPawn ? AbilitySystemPawn->GetAbilitySystemComponent() : nullptr;
}

void UWxAbilitySlotSwitcher::WatchSlot(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InSlotTags, const UGameplayAbility* InAbility, UWxViewModel_Ability* InViewModel)
{
	if (!InASC || InSlotTags.IsEmpty() || !InViewModel)
	{
		return;
	}

	Shutdown();

	CachedASC = InASC;
	SlotTags = InSlotTags;
	CurrentAbility = InAbility;
	CurrentViewModel = InViewModel;
	SourceName = NAME_None;

	Subscribe();
}

bool UWxAbilitySlotSwitcher::HandlesSlot(const FGameplayTagContainer& InSlotTags) const
{
	return SlotTags == InSlotTags;
}

void UWxAbilitySlotSwitcher::Construct()
{
	Super::Construct();

	// 리졸버는 첫 생성 때만 불리므로, 위젯이 다시 구성될 때 구독을 되살리는 것은 여기가 맡는다.
	if (SlotTags.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* ASC = FindAbilitySystem(GetUserWidget());
	if (!ASC)
	{
		return;
	}

	// 빙의 폰이 바뀌었으면 보던 후보를 버리고 새 ASC 기준으로 다시 고른다.
	if (ASC != CachedASC.Get())
	{
		Shutdown();
		CachedASC = ASC;
		CurrentAbility.Reset();
	}

	Subscribe();
	RequestRefresh();
}

void UWxAbilitySlotSwitcher::Destruct()
{
	Shutdown();

	Super::Destruct();
}

void UWxAbilitySlotSwitcher::BeginDestroy()
{
	// 위젯이 Construct 없이 사라지면 Destruct 가 오지 않으므로 여기서도 마감한다.
	Shutdown();

	Super::BeginDestroy();
}

void UWxAbilitySlotSwitcher::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RequestRefresh();
}

void UWxAbilitySlotSwitcher::HandleAbilitySpecDirtied(const FGameplayAbilitySpec& Spec)
{
	RequestRefresh();
}

void UWxAbilitySlotSwitcher::Subscribe()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	ASC->AbilitySpecDirtiedCallbacks.RemoveAll(this);

	ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxAbilitySlotSwitcher::HandleTagChanged);
	ASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UWxAbilitySlotSwitcher::HandleAbilitySpecDirtied);
}

void UWxAbilitySlotSwitcher::RequestRefresh()
{
	if (RefreshHandle.IsValid())
	{
		return;
	}

	RefreshHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWxAbilitySlotSwitcher::FlushRefresh)
	);
}

bool UWxAbilitySlotSwitcher::FlushRefresh(float DeltaTime)
{
	RefreshHandle.Reset();

	UAbilitySystemComponent* ASC = CachedASC.Get();
	UUserWidget* OwningWidget = GetUserWidget();
	if (!ASC || !OwningWidget)
	{
		return false;
	}

	const UGameplayAbility* PickedAbility = PickAbility(*ASC, SlotTags, CurrentAbility.Get());
	if (!PickedAbility || PickedAbility == CurrentAbility.Get())
	{
		return false;
	}

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);
	UWxViewModel_Ability* NewViewModel = AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAbilityViewModel(PickedAbility->GetAssetTags()) : nullptr;
	if (!NewViewModel)
	{
		return false;
	}

	if (NewViewModel == CurrentViewModel.Get())
	{
		// 이미 이 뷰모델을 그리고 있다. 표시는 맞으므로 고른 것만 기억한다.
		CurrentAbility = PickedAbility;
		return false;
	}

	UMVVMView* View = OwningWidget->GetExtension<UMVVMView>();
	if (!View)
	{
		return false;
	}

	if (SourceName.IsNone())
	{
		SourceName = FindSourceName(*View);
	}

	// 소스가 세터를 만들어 두지 않았으면 엔진이 교체를 거부한다. 고른 것을 확정하지 않고 돌아가 다음 통지에 다시 시도한다.
	if (SourceName.IsNone() || !View->SetViewModel(SourceName, NewViewModel))
	{
		return false;
	}

	CurrentAbility = PickedAbility;
	CurrentViewModel = NewViewModel;

	return false;
}

FName UWxAbilitySlotSwitcher::FindSourceName(const UMVVMView& View) const
{
	const UMVVMViewClass* ViewClass = View.GetViewClass();
	const UWxViewModel_Ability* ViewModel = CurrentViewModel.Get();
	if (!ViewClass || !ViewModel)
	{
		return NAME_None;
	}

	FName FoundName = NAME_None;
	for (const FMVVMView_Source& Source : View.GetSources())
	{
		if (Source.Source.Get() != ViewModel)
		{
			continue;
		}

		// 두 슬롯이 같은 뷰모델로 풀렸다면 어느 쪽이 내 자리인지 가릴 근거가 없다. 남의 슬롯을 갈아 끼우느니 멈춘다.
		if (!FoundName.IsNone())
		{
			return NAME_None;
		}

		FoundName = ViewClass->GetSource(Source.ClassKey).GetName();
	}

	return FoundName;
}

void UWxAbilitySlotSwitcher::Shutdown()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
		ASC->AbilitySpecDirtiedCallbacks.RemoveAll(this);
	}

	if (RefreshHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RefreshHandle);
		RefreshHandle.Reset();
	}
}
