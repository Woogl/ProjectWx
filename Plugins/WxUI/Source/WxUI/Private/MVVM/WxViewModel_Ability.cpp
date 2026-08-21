// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/Texture2D.h"
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

	int32 AbilityMaxRecharges = 1;
	if (const UGameplayEffect* CooldownGE = InAbility->GetCooldownGameplayEffect())
	{
		CachedCooldownClass = CooldownGE->GetClass();
		AbilityMaxRecharges = FMath::Max(1, CooldownGE->StackLimitCount);
	}

	SetMaxRecharges(AbilityMaxRecharges);
	SetHasMultipleCharges(AbilityMaxRecharges > 1);
	SetCurrentCharges(AbilityMaxRecharges);

	if (CachedCooldownClass)
	{
		InASC->OnActiveGameplayEffectAddedDelegateToSelf
			.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);
	}

	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_Ability::HandleTagChanged);

	GetCost(InASC, InAbility);

	RefreshActivationState();

	SeedActiveCooldown();
}

void UWxViewModel_Ability::SeedActiveCooldown()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !CachedCooldownClass)
	{
		return;
	}

	const UGameplayAbility* AbilityCDO = CachedAbility.Get();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = CachedCooldownClass;

	// UpdateCooldownState 는 CooldownDuration 을 기준으로 진행률을 내는데, 그 값은 GE 적용 통지에서만 채워진다.
	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC->GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle);
		if (!ActiveGE || ActiveGE->Spec.GetEffectContext().GetAbility() != AbilityCDO)
		{
			continue;
		}

		const float SpecDuration = ActiveGE->Spec.GetDuration();
		if (SpecDuration > 0.f)
		{
			SetCooldownDuration(SpecDuration);
			break;
		}
	}

	if (CooldownDuration <= 0.f)
	{
		// 돌고 있는 쿨다운이 없다 — Initialize 가 세운 만충 상태가 그대로 옳다.
		return;
	}

	SetIsOnCooldown(true);

	if (UpdateCooldownState(0.f) && !TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UWxViewModel_Ability::UpdateCooldownState)
		);
	}
}

void UWxViewModel_Ability::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);

		if (CostAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CostAttribute).RemoveAll(this);
		}
		if (CostMaxAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CostMaxAttribute).RemoveAll(this);
		}
	}

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	CachedASC.Reset();
	CachedAbility.Reset();
	CachedCooldownClass = nullptr;
	CostAttribute = FGameplayAttribute();
	CostMaxAttribute = FGameplayAttribute();

	Super::Deinitialize();
}

bool UWxViewModel_Ability::TryActivateAbility()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const UGameplayAbility* AbilityCDO = CachedAbility.Get();
	if (!ASC || !AbilityCDO)
	{
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability.Get() == AbilityCDO)
		{
			return ASC->TryActivateAbility(Spec.Handle);
		}
	}

	return false;
}

const UGameplayAbility* UWxViewModel_Ability::GetBoundAbility() const
{
	return CachedAbility.Get();
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

int32 UWxViewModel_Ability::GetMaxRecharges() const
{
	return MaxRecharges;
}

void UWxViewModel_Ability::SetMaxRecharges(int32 NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxRecharges, NewValue);
}

bool UWxViewModel_Ability::GetHasMultipleCharges() const
{
	return HasMultipleCharges;
}

void UWxViewModel_Ability::SetHasMultipleCharges(bool NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(HasMultipleCharges, NewValue);
}

bool UWxViewModel_Ability::GetCanActivate() const
{
	return CanActivate;
}

void UWxViewModel_Ability::SetCanActivate(bool NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CanActivate, NewValue);
}

bool UWxViewModel_Ability::GetCheckCost() const
{
	return CheckCost;
}

void UWxViewModel_Ability::SetCheckCost(bool NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CheckCost, NewValue);
}

float UWxViewModel_Ability::GetCostAmount() const
{
	return CostAmount;
}

void UWxViewModel_Ability::SetCostAmount(float NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CostAmount, NewValue);
}

UObject* UWxViewModel_Ability::GetIcon() const
{
	return Icon;
}

void UWxViewModel_Ability::SetIcon(UObject* NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, NewValue);
}

void UWxViewModel_Ability::SetIconSoft(const TSoftObjectPtr<UObject>& InIcon)
{
	RequestImageAsync(TEXT("Icon"), InIcon);
}

void UWxViewModel_Ability::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	SetIcon(LoadedImage);
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
	if (SpecDuration > 0.f && CooldownDuration <= 0.f)
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

	// 쿨다운 GE는 태그를 부여하지 않아 태그 이벤트로 감지되지 않으므로 여기서 재평가한다
	RefreshActivationState();
}

void UWxViewModel_Ability::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshActivationState();
}

void UWxViewModel_Ability::HandleCostAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 자원 값 자체는 UWxViewModel_Attribute 가 같은 어트리뷰트를 구독해 갱신한다. 여기서는 발동 가능 여부만 다시 본다.
	RefreshActivationState();
}

bool UWxViewModel_Ability::UpdateCooldownState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !CachedCooldownClass || CooldownDuration <= 0.f)
	{
		// false 반환은 티커를 제거하므로 핸들도 함께 비운다.
		// 남겨 두면 재등록 게이트(!TickerHandle.IsValid())가 닫힌 채로 굳어 쿨다운 갱신이 영구 정지한다.
		TickerHandle.Reset();
		return false;
	}

	const UGameplayAbility* AbilityCDO = CachedAbility.Get();
	const UWorld* World = ASC->GetWorld();
	if (!World)
	{
		TickerHandle.Reset();
		return false;
	}
	const float WorldTime = World->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = CachedCooldownClass;

	// 활성 쿨다운 GE 1개 = 회복 대기 중인 충전 1개.
	// 가장 먼저 만료될 GE가 다음 충전 회복 시점이다.
	int32 ConsumedCharges = 0;
	float NextChargeRemaining = 0.f;
	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC->GetActiveEffects(Query))
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle))
		{
			if (ActiveGE->Spec.GetEffectContext().GetAbility() == AbilityCDO)
			{
				// 만료됐지만 아직 제거되지 않은 GE(클라이언트는 제거가 리플리케이션으로 도착할 때까지 지연됨)는 회복된 충전으로 취급한다
				const float Remaining = (ActiveGE->StartWorldTime + ActiveGE->Spec.GetDuration()) - WorldTime;
				if (Remaining <= 0.f)
				{
					continue;
				}

				if (ConsumedCharges == 0 || Remaining < NextChargeRemaining)
				{
					NextChargeRemaining = Remaining;
				}
				++ConsumedCharges;
			}
		}
	}

	if (ConsumedCharges == 0)
	{
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		SetCurrentCharges(MaxRecharges);
		TickerHandle.Reset();
		RefreshActivationState();
		return false;
	}

	SetCooldownRemaining(NextChargeRemaining);
	SetCooldownPercent(NextChargeRemaining / CooldownDuration);
	SetCurrentCharges(FMath::Max(0, MaxRecharges - ConsumedCharges));

	// 충전 회복은 별도 이벤트가 없으므로 쿨다운 진행 중에는 매 틱 재평가한다
	RefreshActivationState();

	return true;
}

void UWxViewModel_Ability::RefreshActivationState()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const UGameplayAbility* AbilityCDO = CachedAbility.Get();
	if (!ASC || !AbilityCDO)
	{
		SetCanActivate(false);
		SetCheckCost(false);
		return;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability.Get() != AbilityCDO)
		{
			continue;
		}

		// 엔진 InternalTryActivateAbility와 동일하게, 인스턴스가 있으면 인스턴스 기준으로 판정한다
		const UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance();
		const UGameplayAbility* CanActivateSource = PrimaryInstance ? PrimaryInstance : AbilityCDO;
		SetCanActivate(CanActivateSource->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get()));
		SetCheckCost(CanActivateSource->CheckCost(Spec.Handle, ASC->AbilityActorInfo.Get()));
		return;
	}

	SetCanActivate(false);
	SetCheckCost(false);
}

void UWxViewModel_Ability::GetCost(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility)
{
	const UGameplayEffect* CostGE = InAbility->GetCostGameplayEffect();
	if (!CostGE)
	{
		return;
	}

	float AbilityLevel = 1.f;
	for (const FGameplayAbilitySpec& Spec : InASC->GetActivatableAbilities())
	{
		if (Spec.Ability.Get() == InAbility)
		{
			AbilityLevel = Spec.Level;
			break;
		}
	}

	// 비용 계산은 소스 어빌리티에서 수치를 읽으므로 컨텍스트에 어빌리티를 실어야 한다.
	FGameplayEffectContextHandle CostContext = InASC->MakeEffectContext();
	CostContext.SetAbility(InAbility);

	FGameplayEffectSpec CostSpec(CostGE, CostContext, AbilityLevel);
	CostSpec.CalculateModifierMagnitudes();

	for (int32 ModifierIndex = 0; ModifierIndex < CostGE->Modifiers.Num(); ++ModifierIndex)
	{
		const FGameplayAttribute& ModifierAttribute = CostGE->Modifiers[ModifierIndex].Attribute;
		const float Magnitude = CostSpec.GetModifierMagnitude(ModifierIndex, true);
		if (!ModifierAttribute.IsValid() || FMath::IsNearlyZero(Magnitude))
		{
			continue;
		}

		CostAttribute = ModifierAttribute;

		// 자원 감산이라 음수로 나온다.
		SetCostAmount(FMath::Abs(Magnitude));
		break;
	}

	if (!CostAttribute.IsValid())
	{
		return;
	}

	// 어트리뷰트 셋은 현재값과 최대값을 Max 접두 이름으로 짝지어 둔다.
	if (const UClass* AttributeSetClass = CostAttribute.GetAttributeSetClass())
	{
		const FName MaxAttributeName(*(TEXT("Max") + CostAttribute.GetName()));
		if (FProperty* MaxAttributeProperty = FindFProperty<FProperty>(AttributeSetClass, MaxAttributeName))
		{
			CostMaxAttribute = FGameplayAttribute(MaxAttributeProperty);
		}
	}

	InASC->GetGameplayAttributeValueChangeDelegate(CostAttribute)
		.AddUObject(this, &UWxViewModel_Ability::HandleCostAttributeChanged);

	if (CostMaxAttribute.IsValid())
	{
		InASC->GetGameplayAttributeValueChangeDelegate(CostMaxAttribute)
			.AddUObject(this, &UWxViewModel_Ability::HandleCostAttributeChanged);
	}
}
