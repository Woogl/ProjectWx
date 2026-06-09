// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Effect.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Component/WxEffectComponent_UIData.h"

void UWxViewModel_Effect::Initialize(UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InHandle, const UWxEffectComponent_UIData* InUIData)
{
	if (!InASC || !InHandle.IsValid() || !InUIData)
	{
		return;
	}

	const FActiveGameplayEffect* ActiveEffect = InASC->GetActiveGameplayEffect(InHandle);
	if (!ActiveEffect)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;
	BoundHandle = InHandle;

	SetEffectName(InUIData->DisplayName);
	UTexture2D* MyIcon = InUIData->Icon.IsNull() ? nullptr : InUIData->Icon.LoadSynchronous();
	SetIcon(MyIcon);
	SetStackCount(ActiveEffect->Spec.GetStackCount());
	
	if (const UGameplayEffect* EffectCDO = InASC->GetGameplayEffectCDO(InHandle))
	{
		const FGameplayTagContainer& AssetTags = EffectCDO->GetAssetTags();
		if (!AssetTags.IsEmpty())
		{
			EffectTag = AssetTags.First();
		}
	}

	CachedDuration = ActiveEffect->GetDuration();
	if (CachedDuration > 0.f)
	{
		const UWorld* World = InASC->GetWorld();
		if (!World)
		{
			return;
		}

		const float CurrentTime = World->GetTimeSeconds();
		const float Remaining = FMath::Max((ActiveEffect->StartWorldTime + CachedDuration) - CurrentTime, 0.f);
		EffectEndTime = CurrentTime + Remaining;

		SetDuration(CachedDuration);
		SetTimeRemaining(Remaining);
		SetTimeRemainingPercent(Remaining / CachedDuration);

		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UWxViewModel_Effect::UpdateEffectState)
		);
	}
}

void UWxViewModel_Effect::Deinitialize()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	CachedASC.Reset();
	BoundHandle.Invalidate();

	Super::Deinitialize();
}

FActiveGameplayEffectHandle UWxViewModel_Effect::GetBoundHandle() const
{
	return BoundHandle;
}

FText UWxViewModel_Effect::GetEffectName() const
{
	return EffectName;
}

void UWxViewModel_Effect::SetEffectName(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(EffectName, NewValue);
}

float UWxViewModel_Effect::GetTimeRemaining() const
{
	return TimeRemaining;
}

void UWxViewModel_Effect::SetTimeRemaining(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(TimeRemaining, NewValue);
}

float UWxViewModel_Effect::GetDuration() const
{
	return Duration;
}

void UWxViewModel_Effect::SetDuration(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Duration, NewValue);
}

float UWxViewModel_Effect::GetTimeRemainingPercent() const
{
	return TimeRemainingPercent;
}

void UWxViewModel_Effect::SetTimeRemainingPercent(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(TimeRemainingPercent, NewValue);
}

int32 UWxViewModel_Effect::GetStackCount() const
{
	return StackCount;
}

void UWxViewModel_Effect::SetStackCount(int32 NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(StackCount, NewValue);
	SetIsStackCountAboveOne(NewValue > 1);
}

bool UWxViewModel_Effect::GetIsStackCountAboveOne() const
{
	return IsStackCountAboveOne;
}

void UWxViewModel_Effect::SetIsStackCountAboveOne(bool bNewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(IsStackCountAboveOne, bNewValue);
}

UTexture2D* UWxViewModel_Effect::GetIcon() const
{
	return Icon;
}

void UWxViewModel_Effect::SetIcon(UTexture2D* NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, NewValue);
}

FGameplayTag UWxViewModel_Effect::GetEffectTag() const
{
	return EffectTag;
}

bool UWxViewModel_Effect::UpdateEffectState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return false;
	}

	const FActiveGameplayEffect* ActiveEffect = ASC->GetActiveGameplayEffect(BoundHandle);
	if (!ActiveEffect)
	{
		SetTimeRemaining(0.f);
		SetTimeRemainingPercent(0.f);
		SetStackCount(0);
		return false;
	}

	SetStackCount(ActiveEffect->Spec.GetStackCount());

	if (CachedDuration > 0.f)
	{
		const UWorld* World = ASC->GetWorld();
		if (!World)
		{
			return false;
		}

		const float CurrentTime = World->GetTimeSeconds();
		const float Remaining = FMath::Max(ActiveEffect->StartWorldTime + CachedDuration - CurrentTime, 0.f);
		SetTimeRemaining(Remaining);
		SetTimeRemainingPercent(FMath::Min(Remaining / CachedDuration, 1.f));
	}

	return true;
}
