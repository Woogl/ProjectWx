// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility, int32 InMaxCharges)
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

	const int32 AbilityMaxCharges = FMath::Max(1, InMaxCharges);
	SetMaxCharges(AbilityMaxCharges);
	SetHasMultipleCharges(AbilityMaxCharges > 1);
	SetCurrentCharges(AbilityMaxCharges);

	const FGameplayTag CooldownTag = GetCooldownTag();
	if (CooldownTag.IsValid())
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

FGameplayTag UWxViewModel_Ability::GetCooldownTag() const
{
	const UGameplayAbility* Ability = CachedAbility.Get();
	if (!Ability)
	{
		return FGameplayTag();
	}

	const FGameplayTagContainer* CooldownTags = Ability->GetCooldownTags();
	if (!CooldownTags || CooldownTags->IsEmpty())
	{
		return FGameplayTag();
	}

	return CooldownTags->First();
}

int32 UWxViewModel_Ability::GetConsumedCharges() const
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const FGameplayTag CooldownTag = GetCooldownTag();
	if (!ASC || !CooldownTag.IsValid())
	{
		return 0;
	}

	// 스태킹 없이 충전 소모마다 독립적인 GE 인스턴스가 생성된다.
	// 각 인스턴스의 스택 수는 1이므로 GetAggregatedStackCount = 활성 GE 인스턴스 수 = 소모된 충전 수.
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(TagContainer);
	return ASC->GetAggregatedStackCount(Query);
}

void UWxViewModel_Ability::HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	const FGameplayTag CooldownTag = GetCooldownTag();
	if (!CooldownTag.IsValid())
	{
		return;
	}

	FGameplayTagContainer GrantedTags;
	SpecApplied.GetAllGrantedTags(GrantedTags);
	if (!GrantedTags.HasTag(CooldownTag))
	{
		return;
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

	const FGameplayTag CooldownTag = GetCooldownTag();
	if (!CooldownTag.IsValid())
	{
		return false;
	}

	// 활성 쿨다운 GE 중 가장 빨리 만료되는 것의 남은 시간을 구한다.
	// 충전 시스템에서 "다음 충전까지 남은 시간"에 해당한다.
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(TagContainer);
	TArray<TPair<float, float>> TimesAndDurations = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	float MinTimeRemaining = 0.f;
	float CorrespondingDuration = 0.f;
	for (const TPair<float, float>& Pair : TimesAndDurations)
	{
		if (MinTimeRemaining <= 0.f || Pair.Key < MinTimeRemaining)
		{
			MinTimeRemaining = Pair.Key;
			CorrespondingDuration = Pair.Value;
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

	SetCooldownDuration(CorrespondingDuration);
	SetCooldownRemaining(MinTimeRemaining);
	SetCooldownPercent(CorrespondingDuration > 0.f ? MinTimeRemaining / CorrespondingDuration : 0.f);

	// 소모된 충전 수 = 활성 GE 인스턴스 수. 이를 뺀 값이 현재 남은 충전 수.
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
