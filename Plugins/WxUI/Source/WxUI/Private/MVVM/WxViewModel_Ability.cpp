// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/Texture2D.h"
#include "GameplayEffect.h"
#include "WxUIData.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InAbilityTags)
{
	// 빈 컨테이너는 HasAll 이 항상 true 라 아무 어빌리티나 매칭되므로 거부한다.
	if (!InASC || InAbilityTags.IsEmpty())
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;
	AbilityTags = InAbilityTags;

	// 어빌리티가 갈려도 쿨다운 GE 는 같은 ASC 에서 오므로 구독은 한 번뿐이다 — 지금 물고 있는 쿨다운 태그로 거르는 것은 핸들러가 한다.
	InASC->OnActiveGameplayEffectAddedDelegateToSelf
		.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);

	// 어빌리티의 블록/필요 태그로 좁힐 수 없다 — 발동 판정에는 배타 그룹 점유도 걸리는데, 그건 태그가 아니라 ASC 내부 상태라 다른 어빌리티의 ActivationOwnedTags 변화로만 감지된다.
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_Ability::HandleTagChanged);

	RefreshBoundAbility();
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

void UWxViewModel_Ability::StopCooldownTicker()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
}

int32 UWxViewModel_Ability::QueryCooldownStacks(const UAbilitySystemComponent& ASC, float WorldTime, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.f;
	OutDuration = 0.f;

	for (auto It = ASC.GetActiveGameplayEffects().CreateConstIterator(); It; ++It)
	{
		const FActiveGameplayEffect& ActiveGE = *It;
		if (!ActiveGE.Spec.Def || !ActiveGE.Spec.Def->GetGrantedTags().HasAny(CachedCooldownTags))
		{
			continue;
		}

		const float Duration = ActiveGE.Spec.GetDuration();
		if (Duration <= 0.f)
		{
			continue;
		}

		// 회복 시점이 지나도 제거 복제가 올 때까지는 소모된 상태 그대로 둔다.
		// 발동 판정도 같은 복제 값을 보므로, 여기서 미리 돌려주면 표시만 앞서가 "게이지는 찼는데 안 나가는" 구간이 생긴다.
		OutRemaining = FMath::Max((ActiveGE.StartWorldTime + Duration) - WorldTime, 0.f);
		OutDuration = Duration;
		return ActiveGE.Spec.GetStackCount();
	}

	return 0;
}

void UWxViewModel_Ability::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);

		UnbindCostAttributes(*ASC);
	}

	StopCooldownTicker();

	if (ActivationRefreshHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ActivationRefreshHandle);
		ActivationRefreshHandle.Reset();
	}

	CachedASC.Reset();
	CachedAbility.Reset();
	AbilityTags.Reset();
	CachedCooldownTags.Reset();

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

void UWxViewModel_Ability::RefreshBoundAbility()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	// 매칭 시멘틱은 엔진의 GetActivatableGameplayAbilitySpecsByAllMatchingTags 와 같다. 여러 어빌리티가 걸리면 첫 번째를 쓴다.
	const UGameplayAbility* MatchedAbility = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasAll(AbilityTags))
		{
			MatchedAbility = Spec.Ability;
			break;
		}
	}

	if (MatchedAbility == CachedAbility.Get())
	{
		return;
	}

	// 옛 어빌리티에 매달린 것부터 끊는다.
	UnbindCostAttributes(*ASC);
	StopCooldownTicker();

	CachedAbility = MatchedAbility;
	CachedCooldownTags.Reset();

	if (!MatchedAbility)
	{
		SetTitle(FText::GetEmpty());
		SetDescription(FText::GetEmpty());
		RequestImageAsync(TEXT("Icon"), nullptr);
		SetCostAmount(0.f);
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		SetMaxRecharges(0);
		SetCurrentCharges(0);
		RefreshActivationState();
		return;
	}

	// 표시 계약을 구현하지 않은 어빌리티는 이름도 아이콘도 없이 단일 충전으로 그려진다.
	int32 NewMaxRecharges = 1;
	SetTitle(FText::GetEmpty());
	SetDescription(FText::GetEmpty());
	if (const IWxUIData* UIData = Cast<IWxUIData>(MatchedAbility))
	{
		SetTitle(UIData->GetTitle());
		SetDescription(UIData->GetDescription());
		NewMaxRecharges = UIData->GetMaxRecharges();

		// 전투 중 동기 로드 히치를 피한다.
		RequestImageAsync(TEXT("Icon"), UIData->GetIcon());
	}
	else
	{
		RequestImageAsync(TEXT("Icon"), nullptr);
	}

	if (const FGameplayTagContainer* CooldownTags = MatchedAbility->GetCooldownTags())
	{
		CachedCooldownTags = *CooldownTags;
	}

	BindCostAttributes(*ASC, *MatchedAbility);
	SetMaxRecharges(NewMaxRecharges);

	// 쿨다운이 없는 어빌리티는 아래 갱신이 첫 줄에서 빠져나가므로 충전을 여기서 채운다.
	if (CachedCooldownTags.IsEmpty())
	{
		SetCurrentCharges(NewMaxRecharges);
	}
	else if (UpdateCooldownState(0.f))
	{
		StartCooldownTicker();
	}

	RefreshActivationState();
}

const FGameplayTagContainer& UWxViewModel_Ability::GetAbilityTags() const
{
	return AbilityTags;
}

FText UWxViewModel_Ability::GetTitle() const
{
	return Title;
}

void UWxViewModel_Ability::SetTitle(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Title, NewValue);
}

FText UWxViewModel_Ability::GetDescription() const
{
	return Description;
}

void UWxViewModel_Ability::SetDescription(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Description, NewValue);
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

void UWxViewModel_Ability::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	SetIcon(LoadedImage);
}

void UWxViewModel_Ability::HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	if (CachedCooldownTags.IsEmpty() || !SpecApplied.Def || !SpecApplied.Def->GetGrantedTags().HasAny(CachedCooldownTags))
	{
		return;
	}

	// 남은 시간·충전 수·진행률 분모는 첫 틱이 채운다 — 방금 적용된 GE 가 활성 목록에 보이는 시점에 기대지 않기 위해서다.
	SetIsOnCooldown(true);
	StartCooldownTicker();

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
	if (!World || CachedCooldownTags.IsEmpty())
	{
		// false 반환은 티커를 제거하므로 핸들도 함께 비운다.
		// 남겨 두면 재등록 게이트(!TickerHandle.IsValid())가 닫힌 채로 굳어 쿨다운 갱신이 영구 정지한다.
		TickerHandle.Reset();
		return false;
	}

	float ChargeRemaining = 0.f;
	float ChargeDuration = 0.f;
	const int32 ConsumedCharges = QueryCooldownStacks(*ASC, World->GetTimeSeconds(), ChargeRemaining, ChargeDuration);

	// GE 가 살아 있는 동안은 스택이 최소 하나라 여기 오지 않는다. 즉 티커는 GE 가 실제로 사라진 뒤에만 멈춘다.
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

	SetIsOnCooldown(true);
	SetCooldownDuration(ChargeDuration);
	SetCooldownRemaining(ChargeRemaining);
	SetCooldownPercent(ChargeRemaining / ChargeDuration);

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

	// 코스트가 없는 어빌리티로 갈아탔을 때 옛 수치가 남지 않도록 구독 여부와 무관하게 먼저 반영한다.
	SetCostAmount(FoundCost);

	if (!FoundCostAttribute.IsValid())
	{
		return;
	}

	CostAttribute = FoundCostAttribute;

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

void UWxViewModel_Ability::UnbindCostAttributes(UAbilitySystemComponent& ASC)
{
	if (CostAttribute.IsValid())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(CostAttribute).RemoveAll(this);
	}
	if (CostMaxAttribute.IsValid())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(CostMaxAttribute).RemoveAll(this);
	}

	CostAttribute = FGameplayAttribute();
	CostMaxAttribute = FGameplayAttribute();
}
