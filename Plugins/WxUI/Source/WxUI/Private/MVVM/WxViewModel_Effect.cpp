// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Effect.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "MVVM/WxViewModelUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UWxViewModel_Effect::Initialize(UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InHandle, const FWxConfigureEffectViewModel& InConfigurePresentation)
{
	if (!InASC || !InHandle.IsValid() || !InConfigurePresentation.IsBound())
	{
		return;
	}

	const FActiveGameplayEffect* ActiveEffect = InASC->GetActiveGameplayEffect(InHandle);
	if (!ActiveEffect || !ActiveEffect->Spec.Def || !InConfigurePresentation.Execute(*this, *ActiveEffect->Spec.Def))
	{
		return;
	}

	CachedASC = InASC;
	BoundHandle = InHandle;

	SetStackCount(ActiveEffect->Spec.GetStackCount());

	// 스택이 쌓여도 적용 통지는 다시 오지 않는다 — GAS 가 기존 스택 분기에서 추가 경로를 건너뛴다.
	if (FOnActiveGameplayEffectStackChange* StackChanged = InASC->OnGameplayEffectStackChangeDelegate(InHandle))
	{
		StackChangeHandle = StackChanged->AddUObject(this, &UWxViewModel_Effect::HandleStackCountChanged);
	}

	const float EffectDuration = ActiveEffect->GetDuration();

	// 무한 지속은 잔량이 줄지 않는다 — 링을 가득 채워 두고 갱신도 걸지 않는다.
	if (EffectDuration == FGameplayEffectConstants::INFINITE_DURATION)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Duration, 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(TimeRemaining, 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(TimeRemainingPercent, 1.f);
		return;
	}

	if (EffectDuration > 0.f && UpdateEffectState())
	{
		StartTimeRemainingTimer();
	}
}

void UWxViewModel_Effect::Deinitialize()
{
	StopTimeRemainingTimer();

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

	UE_MVVM_SET_PROPERTY_VALUE(Title, FText::GetEmpty());
	UE_MVVM_SET_PROPERTY_VALUE(Description, FText::GetEmpty());
	SetIcon(nullptr);
	UE_MVVM_SET_PROPERTY_VALUE(Duration, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(TimeRemaining, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(TimeRemainingPercent, 0.f);
	SetStackCount(0);
}

FActiveGameplayEffectHandle UWxViewModel_Effect::GetBoundHandle() const
{
	return BoundHandle;
}

void UWxViewModel_Effect::SetStackCount(int32 NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(StackCount, NewValue);
	UE_MVVM_SET_PROPERTY_VALUE(IsStackCountAboveOne, NewValue > 1);
}

void UWxViewModel_Effect::HandleStackCountChanged(FActiveGameplayEffectHandle Handle, int32 NewStackCount, int32 PreviousStackCount)
{
	SetStackCount(NewStackCount);
}

void UWxViewModel_Effect::StartTimeRemainingTimer()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	TimeRemainingTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UWxViewModel_Effect::HandleTimeRemainingTimer);
}

void UWxViewModel_Effect::StopTimeRemainingTimer()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;
	if (World)
	{
		World->GetTimerManager().ClearTimer(TimeRemainingTimerHandle);
	}
	TimeRemainingTimerHandle.Invalidate();
}

void UWxViewModel_Effect::HandleTimeRemainingTimer()
{
	// 실행 중인 단발 예약을 놓아야 다음 월드 틱을 예약할 수 있다.
	TimeRemainingTimerHandle.Invalidate();
	if (UpdateEffectState())
	{
		StartTimeRemainingTimer();
	}
}

bool UWxViewModel_Effect::UpdateEffectState()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return false;
	}

	const FActiveGameplayEffect* ActiveEffect = ASC->GetActiveGameplayEffect(BoundHandle);
	if (!ActiveEffect)
	{
		UE_MVVM_SET_PROPERTY_VALUE(TimeRemaining, 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(TimeRemainingPercent, 0.f);
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

		const float Remaining = FMath::Max(ActiveEffect->GetTimeRemaining(World->GetTimeSeconds()), 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(Duration, EffectDuration);
		UE_MVVM_SET_PROPERTY_VALUE(TimeRemaining, Remaining);
		UE_MVVM_SET_PROPERTY_VALUE(TimeRemainingPercent, FMath::Min(Remaining / EffectDuration, 1.f));
	}

	return true;
}

void UWxViewModel_Effect::SetIcon(const TSoftObjectPtr<UObject>& InIcon)
{
	WxViewModel::RequestImageAsync(*this, IconHandle, InIcon, [this](UObject* LoadedIcon)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedIcon);
	});
}

void UWxViewModel_Effect::SetPresentation(const FText& InTitle, const FText& InDescription, const TSoftObjectPtr<UObject>& InIcon)
{
	UE_MVVM_SET_PROPERTY_VALUE(Title, InTitle);
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription);
	SetIcon(InIcon);
}
