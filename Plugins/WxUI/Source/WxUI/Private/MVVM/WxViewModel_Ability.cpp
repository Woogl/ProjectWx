// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility)
{
	if (!InASC || !InAbility)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;
	CachedAbility = InAbility;

	const FGameplayTagContainer& AssetTags = InAbility->GetAssetTags();
	if (!AssetTags.IsEmpty())
	{
		AbilityTag = AssetTags.First();
	}

	// CooldownGameplayEffectClass의 StackLimitCount = 최대 충전 수.
	int32 AbilityMaxCharges = 1;
	if (const TSubclassOf<UGameplayEffect> CooldownClass = GetCooldownEffectClass())
	{
		const UGameplayEffect* CooldownCDO = CooldownClass->GetDefaultObject<UGameplayEffect>();
		AbilityMaxCharges = FMath::Max(1, CooldownCDO->StackLimitCount);
	}

	SetMaxCharges(AbilityMaxCharges);
	SetHasMultipleCharges(AbilityMaxCharges > 1);
	SetCurrentCharges(AbilityMaxCharges);

	if (GetCooldownEffectClass())
	{
		InASC->OnActiveGameplayEffectAddedDelegateToSelf
			.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);
	}
}

void UWxViewModel_Ability::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
	}

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	CachedASC.Reset();
	CachedAbility.Reset();

	Super::Deinitialize();
}

TSubclassOf<UGameplayEffect> UWxViewModel_Ability::GetCooldownEffectClass() const
{
	const UGameplayAbility* Ability = CachedAbility.Get();
	if (!Ability)
	{
		return nullptr;
	}

	const UGameplayEffect* CooldownGE = Ability->GetCooldownGameplayEffect();
	return CooldownGE ? CooldownGE->GetClass() : nullptr;
}

int32 UWxViewModel_Ability::GetConsumedCharges() const
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const TSubclassOf<UGameplayEffect> CooldownClass = GetCooldownEffectClass();
	if (!ASC || !CooldownClass)
	{
		return 0;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = CooldownClass;

	int32 Consumed = 0;
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle))
		{
			Consumed += ActiveGE->Spec.GetStackCount();
		}
	}
	return Consumed;
}

void UWxViewModel_Ability::HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	const TSubclassOf<UGameplayEffect> CooldownClass = GetCooldownEffectClass();
	if (!CooldownClass || SpecApplied.Def == nullptr || SpecApplied.Def->GetClass() != CooldownClass)
	{
		return;
	}

	const float SpecDuration = SpecApplied.GetDuration();
	if (SpecDuration > 0.f)
	{
		SetCooldownDuration(SpecDuration);
	}

	SetIsOnCooldown(true);

	if (!TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UWxViewModel_Ability::UpdateCooldownState)
		);
	}
}

bool UWxViewModel_Ability::UpdateCooldownState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return false;
	}

	const TSubclassOf<UGameplayEffect> CooldownClass = GetCooldownEffectClass();
	if (!CooldownClass)
	{
		return false;
	}

	// 활성 쿨다운 GE의 남은 시간 = 다음 충전 복구까지 남은 시간.
	FGameplayEffectQuery Query;
	Query.EffectDefinition = CooldownClass;
	const TArray<TPair<float, float>> TimesAndDurations = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	float MinTimeRemaining = 0.f;
	for (const TPair<float, float>& Pair : TimesAndDurations)
	{
		if (MinTimeRemaining <= 0.f || Pair.Key < MinTimeRemaining)
		{
			MinTimeRemaining = Pair.Key;
		}
	}

	if (MinTimeRemaining <= 0.f)
	{
		// 모든 쿨다운 GE 만료: 모든 충전 복구 후 ticker 종료
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		SetCurrentCharges(MaxCharges);
		TickerHandle.Reset();
		return false;
	}

	SetCooldownRemaining(MinTimeRemaining);
	SetCooldownPercent(CooldownDuration > 0.f ? MinTimeRemaining / CooldownDuration : 0.f);

	if (MaxCharges > 1)
	{
		const int32 Consumed = GetConsumedCharges();
		SetCurrentCharges(FMath::Max(0, MaxCharges - Consumed));
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

bool UWxViewModel_Ability::GetHasMultipleCharges() const
{
	return HasMultipleCharges;
}

void UWxViewModel_Ability::SetHasMultipleCharges(bool NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(HasMultipleCharges, NewValue);
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
