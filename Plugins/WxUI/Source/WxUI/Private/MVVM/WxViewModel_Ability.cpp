// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "WxUIData.h"
#include "WxGameplayTags.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InAbilityTags)
{
	// 호출자가 현재 슬롯의 태그를 다시 전달할 수도 있으므로 종료 전에 복사한다.
	const FGameplayTagContainer NewAbilityTags = InAbilityTags;
	Deinitialize();
	if (!InASC || NewAbilityTags.IsEmpty())
	{
		return;
	}

	CachedASC = InASC;
	AbilityTags = NewAbilityTags;

	// 어빌리티가 갈려도 쿨다운 GE 는 같은 ASC 에서 오므로 구독은 한 번뿐이다 — 지금 물고 있는 쿨다운 태그로 거르는 것은 핸들러가 한다.
	InASC->OnActiveGameplayEffectAddedDelegateToSelf
		.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);

	// 다른 어빌리티의 발동·종료도 배타 점유를 바꾸므로 특정 슬롯의 블록/필요 태그로 구독을 좁히지 않는다.
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_Ability::HandleTagChanged);
	ActionPhaseChangedHandle = InASC->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(WxGameplayTags::Event_Ability_ActionPhaseChanged),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UWxViewModel_Ability::HandleActionPhaseChanged));

	RefreshBoundAbility();
}

void UWxViewModel_Ability::StartCooldownTimer()
{
	if (CooldownTimerHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	CooldownTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UWxViewModel_Ability::HandleCooldownTimer);
}

void UWxViewModel_Ability::StopCooldownTimer()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;
	if (World)
	{
		World->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}
	CooldownTimerHandle.Invalidate();
}

void UWxViewModel_Ability::HandleCooldownTimer()
{
	// 실행 중인 단발 예약을 놓아야 다음 월드 틱을 예약할 수 있다.
	CooldownTimerHandle.Invalidate();
	if (UpdateCooldownState())
	{
		StartCooldownTimer();
	}
	else
	{
		StopCooldownTimer();
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
		ASC->RemoveGameplayEventTagContainerDelegate(
			FGameplayTagContainer(WxGameplayTags::Event_Ability_ActionPhaseChanged), ActionPhaseChangedHandle);

		UnbindCostAttributes(*ASC);

		if (UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(ActivationRefreshHandle);
		}
	}

	StopCooldownTimer();
	ActionPhaseChangedHandle.Reset();

	CachedASC.Reset();
	CachedAbility.Reset();
	AbilityTags.Reset();
	CachedCooldownTags.Reset();

	Super::Deinitialize();
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		SetTitle(FText::GetEmpty());
		SetDescription(FText::GetEmpty());
		SetIcon(nullptr);
		SetCostAmount(0.f);
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		SetMaxRecharges(0);
		SetCurrentCharges(0);
		SetCanActivate(false);
		SetCheckCost(false);
	}
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

	// 쿨다운과 비용은 보지 않아 표시가 그것들로 흔들리지 않는다.
	const UGameplayAbility* MatchedAbility = nullptr;
	const UGameplayAbility* FallbackAbility = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.Ability->GetAssetTags().HasAll(AbilityTags))
		{
			continue;
		}

		if (Spec.Ability->DoesAbilitySatisfyTagRequirements(*ASC))
		{
			MatchedAbility = Spec.Ability;
			break;
		}

		// 사망처럼 후보가 전부 막히는 구간에는 보던 얼굴을 유지한다.
		if (!FallbackAbility || Spec.Ability.Get() == CachedAbility.Get())
		{
			FallbackAbility = Spec.Ability;
		}
	}

	if (!MatchedAbility)
	{
		MatchedAbility = FallbackAbility;
	}

	if (MatchedAbility == CachedAbility.Get())
	{
		return;
	}

	UnbindCostAttributes(*ASC);
	StopCooldownTimer();

	CachedAbility = MatchedAbility;
	CachedCooldownTags.Reset();
	SetCooldownDuration(0.f);
	SetCooldownRemaining(0.f);
	SetCooldownPercent(0.f);
	SetIsOnCooldown(false);

	if (!MatchedAbility)
	{
		SetTitle(FText::GetEmpty());
		SetDescription(FText::GetEmpty());
		RequestImageAsync(GET_MEMBER_NAME_CHECKED(UWxViewModel_Ability, Icon), nullptr);
		SetCostAmount(0.f);
		SetMaxRecharges(0);
		SetCurrentCharges(0);
		RefreshActivationState();
		return;
	}

	int32 NewMaxRecharges = 1;
	SetTitle(FText::GetEmpty());
	SetDescription(FText::GetEmpty());
	if (const IWxUIData* UIData = Cast<IWxUIData>(MatchedAbility))
	{
		SetTitle(UIData->GetTitle());
		SetDescription(UIData->GetDescription());
		NewMaxRecharges = UIData->GetMaxRecharges();

		// 전투 중 동기 로드 히치를 피한다.
		RequestImageAsync(GET_MEMBER_NAME_CHECKED(UWxViewModel_Ability, Icon), UIData->GetIcon());
	}
	else
	{
		RequestImageAsync(GET_MEMBER_NAME_CHECKED(UWxViewModel_Ability, Icon), nullptr);
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
	else if (UpdateCooldownState())
	{
		StartCooldownTimer();
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
	if (FieldName == GET_MEMBER_NAME_CHECKED(UWxViewModel_Ability, Icon))
	{
		SetIcon(LoadedImage);
	}
}

void UWxViewModel_Ability::HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	if (CachedCooldownTags.IsEmpty() || !SpecApplied.Def || !SpecApplied.Def->GetGrantedTags().HasAny(CachedCooldownTags))
	{
		return;
	}

	// 남은 시간·충전 수·진행률 분모는 첫 틱이 채운다 — 방금 적용된 GE 가 활성 목록에 보이는 시점에 기대지 않기 위해서다.
	SetIsOnCooldown(true);
	StartCooldownTimer();

	RefreshActivationState();
}

void UWxViewModel_Ability::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	ScheduleActivationRefresh();
}

void UWxViewModel_Ability::HandleActionPhaseChanged(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	// 컨테이너 구독은 하위 태그도 받지만 이 계약은 전용 이벤트만 처리한다.
	if (EventTag == WxGameplayTags::Event_Ability_ActionPhaseChanged)
	{
		ScheduleActivationRefresh();
	}
}

void UWxViewModel_Ability::ScheduleActivationRefresh()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;

	// 태그 알림과 단계 전환 뒤 입력 버퍼 재발동이 겹쳐도 최종 상태만 한 번 판정한다.
	if (!World || World->GetTimerManager().IsTimerActive(ActivationRefreshHandle))
	{
		return;
	}

	ActivationRefreshHandle = World->GetTimerManager().SetTimerForNextTick(this, &UWxViewModel_Ability::FlushActivationRefresh);
}

void UWxViewModel_Ability::HandleCostAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 엔진은 값이 그대로여도 통지한다 — 자원이 가득 찬 채 도는 주기 회복 GE 가 매 주기 같은 값을 보낸다.
	if (Data.NewValue == Data.OldValue)
	{
		return;
	}

	// 자원 값 자체는 UWxViewModel_Attribute 가 같은 어트리뷰트를 구독해 갱신한다.
	RefreshActivationState();
}

bool UWxViewModel_Ability::UpdateCooldownState()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const UWorld* World = ASC ? ASC->GetWorld() : nullptr;
	if (!World || CachedCooldownTags.IsEmpty())
	{
		return false;
	}

	float ChargeRemaining = 0.f;
	float ChargeDuration = 0.f;
	const int32 ConsumedCharges = QueryCooldownStacks(*ASC, World->GetTimeSeconds(), ChargeRemaining, ChargeDuration);

	// GE 가 살아 있는 동안은 스택이 최소 하나라 여기 오지 않는다. 갱신은 GE 가 실제로 사라진 뒤에만 멈춘다.
	if (ConsumedCharges == 0)
	{
		SetCooldownDuration(0.f);
		SetCooldownRemaining(0.f);
		SetCooldownPercent(0.f);
		SetIsOnCooldown(false);
		SetCurrentCharges(MaxRecharges);
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

void UWxViewModel_Ability::FlushActivationRefresh()
{
	ActivationRefreshHandle.Invalidate();

	// 후보를 가르는 요건이 태그라 대상부터 다시 고른다. 고른 것이 그대로면 조기 반환한다.
	RefreshBoundAbility();

	RefreshActivationState();
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
