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

	// 쿨다운 GE 적용 시점을 직접 감지한다.
	// GE 클래스가 아니라 스펙이 부여하는 태그로 필터링하므로,
	// CooldownGameplayEffectClass 없이 ApplyCooldown에서 동적으로 GE를 만드는 방식도 지원된다.
	const FGameplayTag CooldownTag = GetCooldownTag();
	if (CooldownTag.IsValid())
	{
		InASC->OnActiveGameplayEffectAddedDelegateToSelf
			.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);
	}

	BindChargeTag(InASC, CooldownTag);
}

void UWxViewModel_Ability::BindChargeTag(UAbilitySystemComponent* InASC, const FGameplayTag& CooldownTag)
{
	if (!CooldownTag.IsValid())
	{
		return;
	}

	// 쿨다운 태그 이름 규칙으로 충전 태그를 유도한다 (Cooldown.X → Charge.X)
	// 해당 태그가 네이티브 태그로 등록되어 있을 때만 충전 시스템이 활성화된다.
	static const FString CooldownPrefix = TEXT("Cooldown.");
	const FString CooldownName = CooldownTag.GetTagName().ToString();
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
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);

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
	CachedAbility.Reset();
	BoundChargeTag = FGameplayTag();

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
		// 쿨다운 종료: ticker 자가 종료 + 상태 초기화
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		TickerHandle.Reset();
		return false;
	}

	SetCooldownDuration(Duration);
	SetCooldownRemaining(TimeRemaining);
	SetCooldownPercent(Duration > 0.f ? TimeRemaining / Duration : 0.f);

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
