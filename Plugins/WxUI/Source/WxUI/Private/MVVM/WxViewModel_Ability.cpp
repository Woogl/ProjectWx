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

	if (const UGameplayEffect* CooldownGE = InAbility->GetCooldownGameplayEffect())
	{
		CachedCooldownClass = CooldownGE->GetClass();
	}

	if (CachedCooldownClass)
	{
		InASC->OnActiveGameplayEffectAddedDelegateToSelf
			.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);
	}

	// 어빌리티의 블록/필요 태그로 좁힐 수 없다 — 발동 판정에는 배타 그룹 점유도 걸리는데, 그건 태그가 아니라 ASC 내부 상태라 다른 어빌리티의 ActivationOwnedTags 변화로만 감지된다.
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_Ability::HandleTagChanged);

	BindCostAttributes(*InASC, *InAbility);

	RefreshActivationState();

	// 최대 충전 수는 게임 모듈이 뒤늦게 채우므로 그전까지는 단일 충전으로 본다.
	SetMaxRecharges(1);
}

void UWxViewModel_Ability::StartCooldownTicker()
{
	if (TickerHandle.IsValid())
	{
		return;
	}

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWxViewModel_Ability::UpdateCooldownState)
	);
}

int32 UWxViewModel_Ability::QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float WorldTime, float& OutNextRemaining, float& OutNextDuration) const
{
	OutNextRemaining = 0.f;
	OutNextDuration = 0.f;

	const UGameplayAbility* AbilityCDO = CachedAbility.Get();

	int32 ActiveCount = 0;
	for (auto It = ASC.GetActiveGameplayEffects().CreateConstIterator(); It; ++It)
	{
		const FActiveGameplayEffect& ActiveGE = *It;
		if (!ActiveGE.Spec.Def || ActiveGE.Spec.Def->GetClass() != CachedCooldownClass)
		{
			continue;
		}

		if (ActiveGE.Spec.GetEffectContext().GetAbility() != AbilityCDO)
		{
			continue;
		}

		// 만료됐지만 아직 제거되지 않은 GE(클라이언트는 제거가 리플리케이션으로 도착할 때까지 지연됨)는 회복된 충전으로 취급한다
		const float SpecDuration = ActiveGE.Spec.GetDuration();
		const float Remaining = (ActiveGE.StartWorldTime + SpecDuration) - WorldTime;
		if (SpecDuration <= 0.f || Remaining <= 0.f)
		{
			continue;
		}

		if (ActiveCount == 0 || Remaining < OutNextRemaining)
		{
			OutNextRemaining = Remaining;
			OutNextDuration = SpecDuration;
		}
		++ActiveCount;
	}

	return ActiveCount;
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

	if (ActivationRefreshHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ActivationRefreshHandle);
		ActivationRefreshHandle.Reset();
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
	SetHasMultipleCharges(NewValue > 1);
	SetCurrentCharges(NewValue);

	// 이 VM 은 UMG 바인딩 최초 평가 시 지연 생성되므로, 쿨다운 도중에 태어나면 GE 적용 통지를 놓쳐 만충으로 굳는다.
	if (UpdateCooldownState(0.f))
	{
		StartCooldownTicker();
	}
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

	// 남은 시간·충전 수·진행률 분모는 첫 틱이 채운다 — 방금 적용된 GE 가 활성 목록에 보이는 시점에 기대지 않기 위해서다.
	SetIsOnCooldown(true);
	StartCooldownTicker();

	// 쿨다운 GE는 태그를 부여하지 않아 태그 이벤트로 감지되지 않으므로 여기서 재평가한다
	RefreshActivationState();
}

void UWxViewModel_Ability::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 통지는 바뀐 태그의 부모까지 오고 GE 하나가 태그를 여럿 부여하므로, 한 프레임에 열댓 번이 몰려도 판정 결과는 마지막 한 번과 같다.
	if (ActivationRefreshHandle.IsValid())
	{
		return;
	}

	ActivationRefreshHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWxViewModel_Ability::FlushActivationRefresh)
	);
}

void UWxViewModel_Ability::HandleCostAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 자원 값 자체는 UWxViewModel_Attribute 가 같은 어트리뷰트를 구독해 갱신한다.
	RefreshActivationState();
}

bool UWxViewModel_Ability::UpdateCooldownState(float DeltaTime)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const UWorld* World = ASC ? ASC->GetWorld() : nullptr;
	if (!World || !CachedCooldownClass)
	{
		// false 반환은 티커를 제거하므로 핸들도 함께 비운다.
		// 남겨 두면 재등록 게이트(!TickerHandle.IsValid())가 닫힌 채로 굳어 쿨다운 갱신이 영구 정지한다.
		TickerHandle.Reset();
		return false;
	}

	// 활성 쿨다운 GE 1개 = 회복 대기 중인 충전 1개.
	float NextChargeRemaining = 0.f;
	float NextChargeDuration = 0.f;
	const int32 ConsumedCharges = QueryActiveCooldowns(*ASC, World->GetTimeSeconds(), NextChargeRemaining, NextChargeDuration);

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

	// 진행률 분모는 충전 1개의 회복 주기다. 회복이 직렬이라 나중에 걸린 GE 일수록 지속시간이 길어지므로, 처음 관측한 값을 쿨다운이 끝날 때까지 유지한다.
	if (CooldownDuration <= 0.f)
	{
		SetCooldownDuration(NextChargeDuration);
	}

	SetIsOnCooldown(true);
	SetCooldownRemaining(NextChargeRemaining);
	SetCooldownPercent(NextChargeRemaining / CooldownDuration);

	// 충전 회복은 별도 이벤트가 없다. 남은 시간과 달리 발동 가능 여부는 충전 수가 실제로 바뀔 때만 달라지므로 그때만 재평가한다.
	const int32 NewCharges = FMath::Max(0, MaxRecharges - ConsumedCharges);
	const bool bChargesChanged = NewCharges != CurrentCharges;
	SetCurrentCharges(NewCharges);

	if (bChargesChanged)
	{
		RefreshActivationState();
	}

	return true;
}

bool UWxViewModel_Ability::FlushActivationRefresh(float DeltaTime)
{
	ActivationRefreshHandle.Reset();
	RefreshActivationState();

	return false;
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

		// 발동 가능이면 엔진이 그 안에서 비용도 이미 통과시켰다. 막혔을 때만 원인이 비용인지 따로 묻는다.
		const bool bCanActivate = CanActivateSource->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());
		SetCanActivate(bCanActivate);
		SetCheckCost(bCanActivate || CanActivateSource->CheckCost(Spec.Handle, ASC->AbilityActorInfo.Get()));
		return;
	}

	SetCanActivate(false);
	SetCheckCost(false);
}

float UWxViewModel_Ability::QueryCost(const UAbilitySystemComponent& ASC, const UGameplayAbility& Ability, FGameplayAttribute& OutCostAttribute) const
{
	OutCostAttribute = FGameplayAttribute();

	const UGameplayEffect* CostGE = Ability.GetCostGameplayEffect();
	if (!CostGE)
	{
		return 0.f;
	}

	float AbilityLevel = 1.f;
	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
	{
		if (Spec.Ability.Get() == &Ability)
		{
			AbilityLevel = Spec.Level;
			break;
		}
	}

	// 비용 계산은 소스 어빌리티에서 수치를 읽으므로 컨텍스트에 어빌리티를 실어야 한다.
	FGameplayEffectContextHandle CostContext = ASC.MakeEffectContext();
	CostContext.SetAbility(&Ability);

	FGameplayEffectSpec CostSpec(CostGE, CostContext, AbilityLevel);
	CostSpec.CalculateModifierMagnitudes();

	for (int32 ModifierIndex = 0; ModifierIndex < CostGE->Modifiers.Num(); ++ModifierIndex)
	{
		const FGameplayAttribute& ModifierAttribute = CostGE->Modifiers[ModifierIndex].Attribute;
		const float Magnitude = CostSpec.GetModifierMagnitude(ModifierIndex);
		if (!ModifierAttribute.IsValid() || FMath::IsNearlyZero(Magnitude))
		{
			continue;
		}

		OutCostAttribute = ModifierAttribute;

		// 자원 감산이라 음수로 나온다.
		return FMath::Abs(Magnitude);
	}

	return 0.f;
}

void UWxViewModel_Ability::BindCostAttributes(UAbilitySystemComponent& ASC, const UGameplayAbility& Ability)
{
	FGameplayAttribute FoundCostAttribute;
	const float FoundCost = QueryCost(ASC, Ability, FoundCostAttribute);
	if (!FoundCostAttribute.IsValid())
	{
		return;
	}

	CostAttribute = FoundCostAttribute;
	SetCostAmount(FoundCost);

	// 어트리뷰트 셋은 현재값과 최대값을 Max 접두 이름으로 짝지어 둔다.
	if (const UClass* AttributeSetClass = CostAttribute.GetAttributeSetClass())
	{
		const FName MaxAttributeName(*(TEXT("Max") + CostAttribute.GetName()));
		if (FProperty* MaxAttributeProperty = FindFProperty<FProperty>(AttributeSetClass, MaxAttributeName))
		{
			CostMaxAttribute = FGameplayAttribute(MaxAttributeProperty);
		}
	}

	ASC.GetGameplayAttributeValueChangeDelegate(CostAttribute)
		.AddUObject(this, &UWxViewModel_Ability::HandleCostAttributeChanged);

	if (CostMaxAttribute.IsValid())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(CostMaxAttribute)
			.AddUObject(this, &UWxViewModel_Ability::HandleCostAttributeChanged);
	}
}
