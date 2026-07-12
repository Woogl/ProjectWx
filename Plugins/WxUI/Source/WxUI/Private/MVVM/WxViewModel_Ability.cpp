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

	AbilityTags = InAbility->GetAssetTags();

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

	// 발동 가능 여부 재평가 트리거 등록.
	// 태그 요건은 ASC 태그 변경으로, 비용은 비용 GE가 수정하는 어트리뷰트 값 변경으로 감지한다.
	// 쿨다운 진행(충전 회복/만료)은 이벤트가 없어 UpdateCooldownState 티커에서 재평가한다.
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_Ability::HandleTagChanged);

	if (const UGameplayEffect* CostGE = InAbility->GetCostGameplayEffect())
	{
		for (const FGameplayModifierInfo& Modifier : CostGE->Modifiers)
		{
			if (Modifier.Attribute.IsValid() && !CostAttributes.Contains(Modifier.Attribute))
			{
				CostAttributes.Add(Modifier.Attribute);
				InASC->GetGameplayAttributeValueChangeDelegate(Modifier.Attribute)
					.AddUObject(this, &UWxViewModel_Ability::HandleCostAttributeChanged);
			}
		}
	}

	RefreshActivationState();
}

void UWxViewModel_Ability::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
		for (const FGameplayAttribute& CostAttribute : CostAttributes)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CostAttribute).RemoveAll(this);
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
	CostAttributes.Reset();

	Super::Deinitialize();
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

UTexture2D* UWxViewModel_Ability::GetIcon() const
{
	return Icon;
}

void UWxViewModel_Ability::SetIcon(UTexture2D* NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, NewValue);
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
	RefreshActivationState();
}

bool UWxViewModel_Ability::UpdateCooldownState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !CachedCooldownClass || CooldownDuration <= 0.f)
	{
		return false;
	}

	const UGameplayAbility* AbilityCDO = CachedAbility.Get();
	const float WorldTime = ASC->GetWorld()->GetTimeSeconds();

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
