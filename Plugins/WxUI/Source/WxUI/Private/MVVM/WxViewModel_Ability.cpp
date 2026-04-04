// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility)
{
	if (!InASC || !InAbility)
	{
		return;
	}
	
	Deinitialize();
	CachedASC = InASC;

	const FGameplayTagContainer& AssetTags = InAbility->GetAssetTags();
	if (!AssetTags.IsEmpty())
	{
		AbilityTag = AssetTags.First();
	}

	const FGameplayTagContainer* CooldownTags = InAbility->GetCooldownTags();
	if (CooldownTags && !CooldownTags->IsEmpty())
	{
		BoundCooldownTag = CooldownTags->First();
		InASC->RegisterGameplayTagEvent(BoundCooldownTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWxViewModel_Ability::HandleCooldownTagChanged);
	}

	BindChargeTag(InASC);
}

void UWxViewModel_Ability::BindChargeTag(UAbilitySystemComponent* InASC)
{
	if (!BoundCooldownTag.IsValid())
	{
		return;
	}

	// 쿨다운 태그 이름 규칙으로 충전 태그를 유도한다 (Cooldown.X → Charge.X)
	// 해당 태그가 네이티브 태그로 등록되어 있을 때만 충전 시스템이 활성화된다.
	static const FString CooldownPrefix = TEXT("Cooldown.");
	const FString CooldownName = BoundCooldownTag.GetTagName().ToString();
	if (!CooldownName.StartsWith(CooldownPrefix))
	{
		return;
	}

	const FString Suffix = CooldownName.Mid(CooldownPrefix.Len());
	BoundChargeTag = FGameplayTag::RequestGameplayTag(FName(*(TEXT("Charge.") + Suffix)), false);
	if (!BoundChargeTag.IsValid())
	{
		return;
	}

	InASC->RegisterGameplayTagEvent(BoundChargeTag, EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &UWxViewModel_Ability::HandleChargeTagChanged);

	const int32 InitialCount = InASC->GetTagCount(BoundChargeTag);
	SetCurrentCharges(InitialCount);
	SetMaxCharges(InitialCount);
}

void UWxViewModel_Ability::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (BoundCooldownTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(BoundCooldownTag, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}

		if (BoundChargeTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(BoundChargeTag, EGameplayTagEventType::AnyCountChange)
				.RemoveAll(this);
		}
	}

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	CachedASC.Reset();
	BoundCooldownTag = FGameplayTag();
	BoundChargeTag = FGameplayTag();

	Super::Deinitialize();
}

void UWxViewModel_Ability::HandleCooldownTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	if (NewCount > 0)
	{
		SetIsOnCooldown(true);

		if (!TickerHandle.IsValid())
		{
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateUObject(this, &UWxViewModel_Ability::UpdateCooldownState)
			);
		}
	}
	else
	{
		// 쿨다운 종료
		if (TickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
			TickerHandle.Reset();
		}

		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
	}

	// 쿨다운 태그 변경 시점에 충전 카운트도 동기화 (쿨다운 시작 = 충전 소모, 쿨다운 종료 = 재충전)
	if (BoundChargeTag.IsValid())
	{
		SetCurrentCharges(ASC->GetTagCount(BoundChargeTag));
	}
}

void UWxViewModel_Ability::HandleChargeTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	SetCurrentCharges(NewCount);

	if (NewCount > MaxCharges)
	{
		SetMaxCharges(NewCount);
	}
}

bool UWxViewModel_Ability::UpdateCooldownState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return false;
	}

	// 매 프레임 ASC의 Active Effect에서 직접 남은 시간을 조회한다.
	// CooldownEndTime 역산 방식은 Prediction Reconciliation으로 GE가 교체될 때 무효화될 수 있다.
	FGameplayEffectQuery Query;
	Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(BoundCooldownTag));

	TArray<float> RemainingTimes = ASC->GetActiveEffectsTimeRemaining(Query);
	TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);

	if (RemainingTimes.Num() > 0)
	{
		int32 BestIdx = 0;
		for (int32 Idx = 1; Idx < RemainingTimes.Num(); ++Idx)
		{
			if (RemainingTimes[Idx] > RemainingTimes[BestIdx])
			{
				BestIdx = Idx;
			}
		}

		SetCooldownDuration(Durations[BestIdx]);
		SetCooldownRemaining(RemainingTimes[BestIdx]);
		SetCooldownPercent(Durations[BestIdx] > 0.f ? RemainingTimes[BestIdx] / Durations[BestIdx] : 0.f);
	}

	if (BoundChargeTag.IsValid())
	{
		SetCurrentCharges(ASC->GetTagCount(BoundChargeTag));
	}

	return true;
}

float UWxViewModel_Ability::GetCooldownRemaining() const
{
	return CooldownRemaining;
}

void UWxViewModel_Ability::SetCooldownRemaining(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CooldownRemaining, NewValue);
}

float UWxViewModel_Ability::GetCooldownDuration() const
{
	return CooldownDuration;
}

void UWxViewModel_Ability::SetCooldownDuration(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CooldownDuration, NewValue);
}

float UWxViewModel_Ability::GetCooldownPercent() const
{
	return CooldownPercent;
}

void UWxViewModel_Ability::SetCooldownPercent(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CooldownPercent, NewValue);
}

bool UWxViewModel_Ability::GetIsOnCooldown() const
{
	return IsOnCooldown;
}

void UWxViewModel_Ability::SetIsOnCooldown(bool NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(IsOnCooldown, NewValue);
}

int32 UWxViewModel_Ability::GetCurrentCharges() const
{
	return CurrentCharges;
}

void UWxViewModel_Ability::SetCurrentCharges(int32 NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, NewValue);
}

int32 UWxViewModel_Ability::GetMaxCharges() const
{
	return MaxCharges;
}

void UWxViewModel_Ability::SetMaxCharges(int32 NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxCharges, NewValue);
}

UTexture2D* UWxViewModel_Ability::GetIcon() const
{
	return Icon;
}

void UWxViewModel_Ability::SetIcon(UTexture2D* NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, NewValue);
}

FGameplayTag UWxViewModel_Ability::GetAbilityTag() const
{
	return AbilityTag;
}
