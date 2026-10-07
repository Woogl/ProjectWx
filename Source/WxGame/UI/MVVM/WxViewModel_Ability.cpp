// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Ability.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "UI/MVVM/WxViewModelUtils.h"
#include "TimerManager.h"

void UWxViewModel_Ability::Initialize(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InAbilityTags)
{
	if (!InASC || InAbilityTags.IsEmpty())
	{
		return;
	}

	CachedASC = InASC;
	AbilityTags = InAbilityTags;

	// 어빌리티가 갈려도 쿨다운 GE 는 같은 ASC 에서 오므로 구독은 한 번뿐이다 — 지금 물고 있는 쿨다운 태그로 거르는 것은 핸들러가 한다.
	InASC->OnActiveGameplayEffectAddedDelegateToSelf
		.AddUObject(this, &UWxViewModel_Ability::HandleGameplayEffectApplied);

	// 후보마다 요건 태그가 다르고 비용 판정도 태그(Effect.IgnoreCosts)를 보므로 특정 태그로 구독을 좁히지 않는다.
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxViewModel_Ability::HandleTagChanged);
	InASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UWxViewModel_Ability::HandleAbilitySpecDirtied);

	RefreshBoundAbility();
	RefreshCheckCost();
}

void UWxViewModel_Ability::Deinitialize()
{
	StopCooldownTimer();
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
		ASC->AbilitySpecDirtiedCallbacks.RemoveAll(this);
		UnbindCostAttributes(*ASC);
		if (UWorld* World = ASC->GetWorld())
		{
			World->GetTimerManager().ClearTimer(ActivationRefreshHandle);
		}
	}
	ActivationRefreshHandle.Invalidate();
	CachedASC.Reset();
	CachedAbility.Reset();
	AbilityTags.Reset();
	CachedCooldownTags.Reset();
	CostAttribute = FGameplayAttribute();

	// 풀에 남은 위젯이 이전 슬롯을 보유해도 발동하거나 늦은 아이콘 로드로 되살아나지 않게 한다.
	SetPresentation(FText::GetEmpty(), FText::GetEmpty(), nullptr, 0);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownRemaining, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownPercent, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(IsOnCooldown, false);
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, 0);
	UE_MVVM_SET_PROPERTY_VALUE(CheckCost, false);
	UE_MVVM_SET_PROPERTY_VALUE(CostAmount, 0.f);
}

bool UWxViewModel_Ability::TryActivateAbility()
{
	const UAbilitySystemComponent* ASC = CachedASC.Get();
	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(CachedAbility.Get());
	if (!ASC || !Ability || !Ability->ActivationInputAction)
	{
		return false;
	}

	const APlayerController* PC = ASC->AbilityActorInfo->PlayerController.Get();
	if (!PC)
	{
		return false;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	if (!InputSubsystem)
	{
		return false;
	}

	// 직접 발동하면 이미 활성인 콤보가 거절하므로, 키와 같은 입력 경로로 넣어 콤보 창과 선입력이 받게 한다.
	InputSubsystem->InjectInputForAction(Ability->ActivationInputAction, FInputActionValue(true), {}, {});
	return true;
}

const FGameplayTagContainer& UWxViewModel_Ability::GetAbilityTags() const
{
	return AbilityTags;
}

void UWxViewModel_Ability::RefreshBoundAbility()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	// 쿨다운과 비용은 보지 않아 표시가 그것들로 흔들리지 않는다.
	const UWxAbilityBase* MatchedAbility = nullptr;
	const UWxAbilityBase* FallbackAbility = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetPrimaryInstance());
		if (!Ability || !Ability->GetAssetTags().HasAll(AbilityTags))
		{
			continue;
		}

		// 재생 중인 액션의 차단까지 보면 액션 도중 바뀐 요건이 액션이 끝날 때까지 표시되지 않으므로 소유자 태그만 본다.
		if (Ability->DoesOwnerSatisfyActivationTags(*ASC))
		{
			MatchedAbility = Ability;
			break;
		}

		// 후보가 전부 요건에 막히는 구간에는 보던 얼굴을 유지한다.
		if (!FallbackAbility || Ability == CachedAbility.Get())
		{
			FallbackAbility = Ability;
		}
	}

	if (!MatchedAbility)
	{
		MatchedAbility = FallbackAbility;
	}

	// 제거된 인스턴스는 Get()이 이미 null이어도 이전 표시를 비워야 한다.
	if (MatchedAbility == CachedAbility.Get() && (MatchedAbility || CachedAbility.IsExplicitlyNull()))
	{
		return;
	}

	UnbindCostAttributes(*ASC);
	StopCooldownTimer();

	CachedAbility = MatchedAbility;
	CachedCooldownTags.Reset();
	UE_MVVM_SET_PROPERTY_VALUE(CooldownRemaining, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownPercent, 0.f);
	UE_MVVM_SET_PROPERTY_VALUE(IsOnCooldown, false);

	if (!MatchedAbility)
	{
		SetPresentation(FText::GetEmpty(), FText::GetEmpty(), nullptr, 0);
		UE_MVVM_SET_PROPERTY_VALUE(CostAmount, 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, 0);
		return;
	}

	SetPresentation(MatchedAbility->GetTitle(), MatchedAbility->GetDescription(), MatchedAbility->GetIcon(), MatchedAbility->GetMaxRecharges());

	if (const FGameplayTagContainer* CooldownTags = MatchedAbility->GetCooldownTags())
	{
		CachedCooldownTags = *CooldownTags;
	}

	BindCostAttributes(*ASC, *MatchedAbility);

	// 쿨다운이 없는 어빌리티는 아래 갱신이 첫 줄에서 빠져나가므로 충전을 여기서 채운다.
	if (CachedCooldownTags.IsEmpty())
	{
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, MaxRecharges);
	}
	else if (UpdateCooldownState())
	{
		StartCooldownTimer();
	}
}

void UWxViewModel_Ability::HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	// 이미 쿨다운 중이면 스택만 늘어 이 알림이 오지 않지만, 그때는 타이머가 이미 돌고 있다.
	if (CachedCooldownTags.IsEmpty() || !SpecApplied.Def || !SpecApplied.Def->GetGrantedTags().HasAny(CachedCooldownTags))
	{
		return;
	}

	// 남은 시간·충전 수는 첫 틱이 채운다 — 방금 적용된 GE 가 활성 목록에 보이는 시점에 기대지 않기 위해서다.
	UE_MVVM_SET_PROPERTY_VALUE(IsOnCooldown, true);
	StartCooldownTimer();
}

void UWxViewModel_Ability::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	ScheduleActivationRefresh();
}

void UWxViewModel_Ability::HandleAbilitySpecDirtied(const FGameplayAbilitySpec& Spec)
{
	ScheduleActivationRefresh();
}

void UWxViewModel_Ability::ScheduleActivationRefresh()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	UWorld* World = ASC ? ASC->GetWorld() : nullptr;

	// 교체(제거 뒤 부여)와 세트 부여는 한 프레임에 알림이 몰리므로 다음 틱에 최종 상태만 한 번 판정한다.
	if (!World || World->GetTimerManager().IsTimerActive(ActivationRefreshHandle))
	{
		return;
	}

	ActivationRefreshHandle = World->GetTimerManager().SetTimerForNextTick(this, &UWxViewModel_Ability::FlushActivationRefresh);
}

void UWxViewModel_Ability::FlushActivationRefresh()
{
	ActivationRefreshHandle.Invalidate();

	// 후보를 가르는 요건이 태그라 대상부터 다시 고른다.
	RefreshBoundAbility();
	RefreshCheckCost();
}

void UWxViewModel_Ability::HandleCostAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 엔진은 값이 그대로여도 통지한다 — 자원이 가득 찬 채 도는 주기 회복 GE 가 매 주기 같은 값을 보낸다.
	if (Data.NewValue == Data.OldValue)
	{
		return;
	}

	// 자원 값 자체는 UWxViewModel_Attribute 가 같은 어트리뷰트를 구독해 갱신한다.
	RefreshCheckCost();
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
		UE_MVVM_SET_PROPERTY_VALUE(CooldownRemaining, 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(CooldownPercent, 0.f);
		UE_MVVM_SET_PROPERTY_VALUE(IsOnCooldown, false);
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, MaxRecharges);
		return false;
	}

	UE_MVVM_SET_PROPERTY_VALUE(IsOnCooldown, true);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownRemaining, ChargeRemaining);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownPercent, ChargeDuration > 0.f ? ChargeRemaining / ChargeDuration : 0.f);

	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, FMath::Max(0, MaxRecharges - ConsumedCharges));

	return true;
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
}

int32 UWxViewModel_Ability::QueryCooldownStacks(const UAbilitySystemComponent& ASC, float WorldTime, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.f;
	OutDuration = 0.f;

	int32 ConsumedCharges = 0;
	for (auto It = ASC.GetActiveGameplayEffects().CreateConstIterator(); It; ++It)
	{
		const FActiveGameplayEffect& ActiveGE = *It;
		if (!ActiveGE.Spec.Def || !ActiveGE.Spec.Def->GetGrantedTags().HasAny(CachedCooldownTags) || ActiveGE.GetDuration() <= 0.f)
		{
			continue;
		}

		// 발동 판정도 스택 제거 복제를 기다리므로 회복 시점이 지나도 그 복제가 올 때까지 소모 상태로 둬야 "게이지는 찼는데 안 나가는" 구간이 없다.
		const float Remaining = FMath::Max(ActiveGE.GetTimeRemaining(WorldTime), 0.f);
		if (ConsumedCharges == 0 || Remaining < OutRemaining)
		{
			OutRemaining = Remaining;
			OutDuration = ActiveGE.GetDuration();
		}
		ConsumedCharges += ActiveGE.Spec.GetStackCount();
	}

	return ConsumedCharges;
}

void UWxViewModel_Ability::RefreshCheckCost()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	const UGameplayAbility* Ability = CachedAbility.Get();
	const FGameplayAbilitySpecHandle Handle = Ability ? Ability->GetCurrentAbilitySpecHandle() : FGameplayAbilitySpecHandle();
	if (!ASC || !ASC->FindAbilitySpecFromHandle(Handle))
	{
		UE_MVVM_SET_PROPERTY_VALUE(CheckCost, false);
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(CheckCost, Ability->CheckCost(Handle, ASC->AbilityActorInfo.Get()));
}

void UWxViewModel_Ability::BindCostAttributes(UAbilitySystemComponent& ASC, const UWxAbilityBase& Ability)
{
	FGameplayAttribute FoundCostAttribute;
	const float FoundCost = Ability.QueryCost(ASC, FoundCostAttribute);

	// 코스트가 없는 어빌리티로 갈아탔을 때 옛 수치가 남지 않도록 구독 여부와 무관하게 먼저 반영한다.
	UE_MVVM_SET_PROPERTY_VALUE(CostAmount, FoundCost);

	if (!FoundCostAttribute.IsValid())
	{
		return;
	}

	CostAttribute = FoundCostAttribute;
	ASC.GetGameplayAttributeValueChangeDelegate(CostAttribute)
		.AddUObject(this, &UWxViewModel_Ability::HandleCostAttributeChanged);
}

void UWxViewModel_Ability::UnbindCostAttributes(UAbilitySystemComponent& ASC)
{
	if (CostAttribute.IsValid())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(CostAttribute).RemoveAll(this);
	}

	CostAttribute = FGameplayAttribute();
}

void UWxViewModel_Ability::SetPresentation(const FText& InTitle, const FText& InDescription, const TSoftObjectPtr<UObject>& InIcon, int32 InMaxRecharges)
{
	UE_MVVM_SET_PROPERTY_VALUE(Title, InTitle);
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription);
	UE_MVVM_SET_PROPERTY_VALUE(MaxRecharges, InMaxRecharges);
	UE_MVVM_SET_PROPERTY_VALUE(HasMultipleCharges, InMaxRecharges > 1);
	WxViewModel::RequestImageAsync(*this, IconHandle, InIcon, [this](UObject* LoadedIcon)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedIcon);
	});
}
