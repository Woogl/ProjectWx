// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Ability.generated.h"

struct FGameplayEventData;
struct FGameplayEffectSpec;
struct FStreamableHandle;
class UAbilitySystemComponent;
class UGameplayAbility;
class UWxAbilityBase;

/**
 * 스킬 슬롯 하나의 뷰모델. 정체성은 어빌리티가 아니라 슬롯을 가리키는 어빌리티 태그다.
 * 그 태그에 맞는 어빌리티가 부여돼 있으면 그것을 물고, 교체되면 갈아타며, 없으면 빈 슬롯으로 남는다.
 * 슬롯 태그를 공유하는 후보가 여럿이면 소유자가 발동 태그 요건을 만족하는 것을 표시하고, 전부 막히면 보던 것을 유지한다. 상황별 가시성은 위젯 바인딩이 맡는다.
 * 무는 대상은 스펙의 기본 인스턴스다 — 엔진 발동 경로(InternalTryActivateAbility)처럼 인스턴스로 판정한다.
 *
 * 쿨다운은 어빌리티의 GetCooldownTags() 로 식별하고, 쿨다운 중에는 월드 타이머로 매 프레임 남은 시간·충전 수를 갱신한다.
 * 소모한 충전 하나가 쿨다운 GE 하나이고 충전은 차례로 돌아오므로, 남은 시간·진행률은 가장 먼저 끝나는 쿨다운(다음 충전) 기준이다.
 *
 * CheckCost 는 ASC 태그·발동 조건 이벤트/비용 어트리뷰트 변화 시점에 재평가된다.
 * 태그 변경과 발동 조건 이벤트는 한 프레임 분을 모아 다음 월드 타이머 틱에 한 번 판정한다.
 * 소모량은 어빌리티를 물 때 한 번만 조회한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Ability : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/**
	 * 생성 직후 한 번만 부른다.
	 * @param InAbilityTags 슬롯을 가리키는 어빌리티 에셋 태그. 비어 있으면 아무 어빌리티나 매칭되므로 거부한다.
	 */
	void Initialize(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InAbilityTags);

	UFUNCTION(BlueprintCallable, Category = "Wx|Ability")
	bool TryActivateAbility();

	/** 물고 있던 어빌리티가 그대로면 아무것도 하지 않는다. */
	void RefreshBoundAbility();

	const FGameplayTagContainer& GetAbilityTags() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	FText Title;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	FText Description;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	float CooldownRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	float CooldownPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	bool IsOnCooldown = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	int32 CurrentCharges = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	int32 MaxRecharges = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	bool HasMultipleCharges = false;

	/** 쿨다운/태그와 무관하게 엔진 CheckCost만으로 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	bool CheckCost = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	float CostAmount = 0.f;

	/** 텍스처 또는 머터리얼이며, 어빌리티를 물 때마다 그것이 든 소프트 참조를 비동기 로드해 세팅한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Ability")
	TObjectPtr<UObject> Icon = nullptr;

private:
	/** 충전이 여럿인지가 이 값에서 파생되므로 함께 갱신된다. */
	void SetMaxRecharges(int32 NewValue);

	void HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleActionPhaseChanged(FGameplayTag EventTag, const FGameplayEventData* Payload);
	void ScheduleActivationRefresh();
	void HandleCostAttributeChanged(const FOnAttributeChangeData& Data);
	bool UpdateCooldownState();
	void HandleCooldownTimer();

	void FlushActivationRefresh();

	void StartCooldownTimer();
	void StopCooldownTimer();

	/**
	 * 쿨다운 태그를 부여하는 활성 GE 수(소모된 충전 수)를 반환하고, 다음 충전까지의 잔여 시간을 낸다.
	 * 순정 조회 API 는 호출마다 배열을 새로 할당하므로, 매 프레임 도는 이 경로에서는 컨테이너를 직접 한 번만 훑는다.
	 */
	int32 QueryCooldownStacks(const UAbilitySystemComponent& ASC, float WorldTime, float& OutRemaining) const;

	void RefreshCheckCost();

	/** 어빌리티에서 조회한 비용을 표시하고, 해당 자원이 바뀌면 비용 판정을 다시 하도록 구독한다. */
	void BindCostAttributes(UAbilitySystemComponent& ASC, const UWxAbilityBase& Ability);

	/** 어빌리티마다 다른 구독이라 어빌리티를 놓을 때마다 푼다. */
	void UnbindCostAttributes(UAbilitySystemComponent& ASC);

	/** 이후 충전·쿨다운 갱신이 이 값을 사용한다. */
	void SetPresentation(const FText& InTitle, const FText& InDescription, const TSoftObjectPtr<UObject>& InIcon, int32 InMaxRecharges, float InCooldownTime);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;

	/** Asset Tags 가 이것을 모두 포함하는(HasAll) 어빌리티를 문다. */
	FGameplayTagContainer AbilityTags;

	/** 비어 있으면 쿨다운이 없는 어빌리티다. */
	FGameplayTagContainer CachedCooldownTags;

	/**
	 * 충전 하나의 회복 시간. 진행률의 분모다.
	 * 뒤 쿨다운 GE 는 앞 쿨다운을 기다린 시간까지 지속시간에 품고 있어 GE 지속시간으로 대신할 수 없다.
	 */
	float CachedCooldownTime = 0.f;

	FGameplayAttribute CostAttribute;

	FTimerHandle CooldownTimerHandle;

	/** 타이머가 활성이면 재평가가 이미 예약돼 있다. 실행 중에도 활성으로 잡히므로 플러시가 먼저 놓는다. */
	FTimerHandle ActivationRefreshHandle;

	TSharedPtr<FStreamableHandle> IconHandle;
};
