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

	// 쿨다운 GE 적용 시점을 직접 감지한다.
	// GE 클래스가 아니라 스펙이 부여하는 태그로 필터링하므로,
	// CooldownGameplayEffectClass 없이 ApplyCooldown에서 동적으로 GE를 만드는 방식도 지원된다.
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

	// 쿨다운 GE의 GE 스택 1개 = 소모된 충전 1회.
	// 쿨다운 태그로 필터링해 ASC에 적용된 모든 매칭 GE의 총 스택 수를 합산한다.
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

	// 이 스펙이 우리 어빌리티의 쿨다운 태그를 부여하는지로 필터링한다.
	// (DynamicGrantedTags + GE Def의 GrantedTags 모두 포함)
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
	const UGameplayAbility* Ability = CachedAbility.Get();
	if (!ASC || !Ability)
	{
		return false;
	}

	// 어빌리티가 자신의 쿨다운 GE를 직접 조회하도록 위임한다.
	// 내부적으로 ASC의 Active Effect를 쿨다운 태그로 쿼리하므로,
	// CooldownEndTime 역산 없이도 Prediction Reconciliation에 안전하다.
	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	if (!ActorInfo)
	{
		return true;
	}

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	Ability->GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle(), ActorInfo, TimeRemaining, Duration);

	if (TimeRemaining <= 0.f)
	{
		// 쿨다운 종료: ticker 자가 종료 + 상태 초기화 + 모든 충전 회복
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		SetCurrentCharges(MaxCharges);
		TickerHandle.Reset();
		return false;
	}

	SetCooldownDuration(Duration);
	SetCooldownRemaining(TimeRemaining);
	SetCooldownPercent(Duration > 0.f ? TimeRemaining / Duration : 0.f);

	// 충전 시스템 어빌리티는 GE 스택 수로부터 남은 충전을 계산
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
