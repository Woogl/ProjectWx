// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Effect.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "WxUIData.h"
#include "Engine/Texture2D.h"

void UWxViewModel_Effect::Initialize(UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InHandle, const IWxUIData* InUIData)
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

	SetTitle(InUIData->GetTitle());
	SetDescription(InUIData->GetDescription());

	// 전투 중 동기 로드 히치를 피한다.
	RequestImageAsync(TEXT("Icon"), InUIData->GetIcon());

	SetStackCount(ActiveEffect->Spec.GetStackCount());

	// 스택이 쌓여도 적용 통지는 다시 오지 않는다 — GAS 가 기존 스택 분기에서 추가 경로를 건너뛴다.
	if (FOnActiveGameplayEffectStackChange* StackChanged = InASC->OnGameplayEffectStackChangeDelegate(InHandle))
	{
		StackChangeHandle = StackChanged->AddUObject(this, &UWxViewModel_Effect::HandleStackCountChanged);
	}

	const float EffectDuration = ActiveEffect->GetDuration();
	if (EffectDuration > 0.f)
	{
		const UWorld* World = InASC->GetWorld();
		if (!World)
		{
			return;
		}

		const float CurrentTime = World->GetTimeSeconds();
		const float Remaining = FMath::Max((ActiveEffect->StartWorldTime + EffectDuration) - CurrentTime, 0.f);

		SetDuration(EffectDuration);
		SetTimeRemaining(Remaining);
		SetTimeRemainingPercent(Remaining / EffectDuration);

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

	// 통지는 활성 효과가 들고 있으므로, 효과가 이미 걷혔으면 조회가 비고 뗄 것도 없다.
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (FOnActiveGameplayEffectStackChange* StackChanged = ASC->OnGameplayEffectStackChangeDelegate(BoundHandle))
		{
			StackChanged->Remove(StackChangeHandle);
		}
	}
	StackChangeHandle.Reset();

	CachedASC.Reset();
	BoundHandle.Invalidate();

	Super::Deinitialize();
}

FActiveGameplayEffectHandle UWxViewModel_Effect::GetBoundHandle() const
{
	return BoundHandle;
}

FText UWxViewModel_Effect::GetTitle() const
{
	return Title;
}

void UWxViewModel_Effect::SetTitle(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Title, NewValue);
}

FText UWxViewModel_Effect::GetDescription() const
{
	return Description;
}

void UWxViewModel_Effect::SetDescription(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Description, NewValue);
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

UObject* UWxViewModel_Effect::GetIcon() const
{
	return Icon;
}

void UWxViewModel_Effect::SetIcon(UObject* NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, NewValue);
}

void UWxViewModel_Effect::HandleStackCountChanged(FActiveGameplayEffectHandle Handle, int32 NewStackCount, int32 PreviousStackCount)
{
	SetStackCount(NewStackCount);
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

	// 스택 재적용이 지속시간을 새로 고친다.
	const float EffectDuration = ActiveEffect->GetDuration();
	if (EffectDuration > 0.f)
	{
		const UWorld* World = ASC->GetWorld();
		if (!World)
		{
			return false;
		}

		const float CurrentTime = World->GetTimeSeconds();
		const float Remaining = FMath::Max(ActiveEffect->StartWorldTime + EffectDuration - CurrentTime, 0.f);
		SetDuration(EffectDuration);
		SetTimeRemaining(Remaining);
		SetTimeRemainingPercent(FMath::Min(Remaining / EffectDuration, 1.f));
	}

	return true;
}

void UWxViewModel_Effect::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	SetIcon(LoadedImage);
}
