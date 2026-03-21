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
	}

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
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
		FGameplayEffectQuery Query;
		Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(BoundCooldownTag));

		TArray<float> RemainingTimes = ASC->GetActiveEffectsTimeRemaining(Query);
		TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);

		if (RemainingTimes.Num() > 0)
		{
			const UWorld* World = ASC->GetWorld();
			if (!World)
			{
				return;
			}

			CachedCooldownDuration = Durations[0];
			CooldownEndTime = World->GetTimeSeconds() + RemainingTimes[0];

			SetCooldownDuration(CachedCooldownDuration);
			SetCooldownRemaining(RemainingTimes[0]);
			SetCooldownPercent(CachedCooldownDuration > 0.f ? RemainingTimes[0] / CachedCooldownDuration : 0.f);
			SetIsOnCooldown(true);

			if (!TickerHandle.IsValid())
			{
				TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
					FTickerDelegate::CreateUObject(this, &UWxViewModel_Ability::UpdateCooldownState)
				);
			}
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

		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
	}
}

bool UWxViewModel_Ability::UpdateCooldownState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return false;
	}

	const UWorld* World = ASC->GetWorld();
	if (!World)
	{
		return false;
	}

	const float Remaining = FMath::Max(CooldownEndTime - World->GetTimeSeconds(), 0.f);
	SetCooldownRemaining(Remaining);
	SetCooldownPercent(CachedCooldownDuration > 0.f ? Remaining / CachedCooldownDuration : 0.f);

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

UTexture2D* UWxViewModel_Ability::GetIcon() const
{
	return Icon;
}

void UWxViewModel_Ability::SetIcon(UTexture2D* NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, NewValue);
}
