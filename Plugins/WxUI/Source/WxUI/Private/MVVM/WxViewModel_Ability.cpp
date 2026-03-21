// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility)
{
	if (!InASC || !InAbility)
	{
		return;
	}

	const FGameplayTagContainer* CooldownTags = InAbility->GetCooldownTags();
	if (!CooldownTags || CooldownTags->IsEmpty())
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;
	BoundCooldownTag = CooldownTags->First();

	InASC->RegisterGameplayTagEvent(BoundCooldownTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxViewModel_Ability::HandleCooldownTagChanged);
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

		if (const UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(CooldownTimerHandle);
		}
	}

	CachedASC.Reset();
	BoundCooldownTag = FGameplayTag();

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
		// 쿨다운 시작: ASC의 Active Effect에서 남은 시간과 전체 시간 추출
		float TimeRemaining = 0.f;
		float TotalDuration = 0.f;

		FGameplayEffectQuery Query;
		Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(BoundCooldownTag));

		TArray<float> RemainingTimes = ASC->GetActiveEffectsTimeRemaining(Query);
		TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);

		if (RemainingTimes.Num() > 0)
		{
			TimeRemaining = RemainingTimes[0];
			TotalDuration = Durations[0];
		}

		SetCooldownDuration(TotalDuration);
		SetCooldownRemaining(TimeRemaining);
		SetCooldownPercent(TotalDuration > 0.f ? TimeRemaining / TotalDuration : 0.f);
		SetbIsOnCooldown(true);

		// 타이머로 매 프레임 갱신
		if (const UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().SetTimer(
				CooldownTimerHandle,
				FTimerDelegate::CreateUObject(this, &UWxViewModel_Ability::UpdateCooldownState),
				0.016f,
				true
			);
		}
	}
	else
	{
		// 쿨다운 종료
		if (const UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(CooldownTimerHandle);
		}

		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetbIsOnCooldown(false);
	}
}

void UWxViewModel_Ability::UpdateCooldownState()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectQuery Query;
	Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(BoundCooldownTag));

	TArray<float> RemainingTimes = ASC->GetActiveEffectsTimeRemaining(Query);
	TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);

	if (RemainingTimes.Num() > 0)
	{
		SetCooldownRemaining(FMath::Max(RemainingTimes[0], 0.f));
		SetCooldownPercent(Durations[0] > 0.f ? FMath::Max(RemainingTimes[0] / Durations[0], 0.f) : 0.f);
	}
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

bool UWxViewModel_Ability::GetbIsOnCooldown() const
{
	return bIsOnCooldown;
}

void UWxViewModel_Ability::SetbIsOnCooldown(bool NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsOnCooldown, NewValue);
}
