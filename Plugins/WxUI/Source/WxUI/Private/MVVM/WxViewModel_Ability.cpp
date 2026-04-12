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

	int32 AbilityMaxCharges = 1;
	if (const UGameplayEffect* CooldownGE = InAbility->GetCooldownGameplayEffect())
	{
		CachedCooldownClass = CooldownGE->GetClass();
		AbilityMaxCharges = FMath::Max(1, CooldownGE->StackLimitCount);
	}

	SetMaxCharges(AbilityMaxCharges);
	SetHasMultipleCharges(AbilityMaxCharges > 1);
	SetCurrentCharges(AbilityMaxCharges);

	if (CachedCooldownClass)
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
	CachedCooldownClass = nullptr;

	Super::Deinitialize();
}

int32 UWxViewModel_Ability::GetConsumedCharges() const
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !CachedCooldownClass)
	{
		return 0;
	}

	const UGameplayAbility* AbilityCDO = CachedAbility.Get();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = CachedCooldownClass;

	int32 Consumed = 0;
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle))
		{
			if (ActiveGE->Spec.GetEffectContext().GetAbility() == AbilityCDO)
			{
				Consumed++;
			}
		}
	}
	return Consumed;
}

void UWxViewModel_Ability::HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	if (!CachedCooldownClass || !SpecApplied.Def || SpecApplied.Def->GetClass() != CachedCooldownClass)
	{
		return;
	}

	if (SpecApplied.GetEffectContext().GetAbility() != CachedAbility.Get())
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
	if (!ASC || !CachedCooldownClass)
	{
		return false;
	}

	const UGameplayAbility* AbilityCDO = CachedAbility.Get();
	const float WorldTime = ASC->GetWorld()->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = CachedCooldownClass;

	float MinTimeRemaining = 0.f;
	int32 ConsumedCharges = 0;
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& ActiveHandle : Handles)
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle))
		{
			if (ActiveGE->Spec.GetEffectContext().GetAbility() != AbilityCDO)
			{
				continue;
			}

			ConsumedCharges++;

			const float Duration = ActiveGE->Spec.GetDuration();
			const float TimeRemaining = (ActiveGE->StartWorldTime + Duration) - WorldTime;
			if (TimeRemaining > 0.f && (MinTimeRemaining <= 0.f || TimeRemaining < MinTimeRemaining))
			{
				MinTimeRemaining = TimeRemaining;
			}
		}
	}

	if (MinTimeRemaining <= 0.f)
	{
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
	SetCurrentCharges(FMath::Max(0, MaxCharges - ConsumedCharges));

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
