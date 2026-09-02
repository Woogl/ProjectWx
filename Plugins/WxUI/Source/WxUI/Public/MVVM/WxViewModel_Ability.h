// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Ability.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UTexture2D;
struct FGameplayEffectSpec;

/**
 * 스킬 슬롯 하나의 뷰모델. 정체성은 어빌리티가 아니라 슬롯을 가리키는 어빌리티 태그다.
 * 그 태그에 맞는 어빌리티가 부여돼 있으면 그것을 물고, 교체되면 갈아타며, 없으면 빈 슬롯으로 남는다.
 *
 * 쿨다운은 어빌리티의 GetCooldownTags() 로 식별하고, 쿨다운 중에는 티커로 매 프레임 남은 시간·충전 수를 갱신한다.
 * 쿨다운 GE 는 소모한 충전 하나를 스택 하나로 쌓으므로, 남은 시간·진행률은 지금 회복 중인 충전 1개 기준이 된다.
 *
 * CanActivate·CheckCost 는 ASC 태그 변경/비용 어트리뷰트 변경/쿨다운 적용·충전 수 변화 시점에 재평가된다.
 * 태그 변경만은 한 프레임 분을 모아 다음 틱에 한 번 판정한다.
 *
 * 소모량은 어빌리티를 물 때 비용 GE 를 한 번 평가해 정한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Ability : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** @param InAbilityTags 슬롯을 가리키는 어빌리티 에셋 태그. 비어 있으면 아무 어빌리티나 매칭되므로 거부한다. */
	void Initialize(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InAbilityTags);
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Wx|Ability")
	bool TryActivateAbility();

	/** 슬롯 태그에 맞는 어빌리티를 다시 찾아 물거나 놓는다. 물고 있던 것이 그대로면 아무것도 하지 않는다. */
	void RefreshBoundAbility();

	/** 이 슬롯의 공유 키다. */
	const FGameplayTagContainer& GetAbilityTags() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	FText Title;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	FText Description;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	float CooldownRemaining = 0.f;

	/** 충전 1개의 회복 시간. 진행률의 분모다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	float CooldownDuration = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	float CooldownPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool IsOnCooldown = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	int32 CurrentCharges = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	int32 MaxRecharges = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool HasMultipleCharges = false;

	/** 비용/쿨다운/태그 요건을 엔진 CanActivateAbility로 종합 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool CanActivate = false;

	/** 쿨다운/태그와 무관하게 엔진 CheckCost만으로 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool CheckCost = false;

	/** 코스트가 없으면 0 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	float CostAmount = 0.f;

	/** 텍스처 또는 머터리얼이며, 어빌리티가 든 소프트 참조를 초기화 때 비동기 로드해 세팅한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	TObjectPtr<UObject> Icon = nullptr;

	FText GetTitle() const;
	void SetTitle(const FText& NewValue);

	FText GetDescription() const;
	void SetDescription(const FText& NewValue);

	float GetCooldownRemaining() const;
	void SetCooldownRemaining(float NewValue);

	float GetCooldownDuration() const;
	void SetCooldownDuration(float NewValue);

	float GetCooldownPercent() const;
	void SetCooldownPercent(float NewValue);

	bool GetIsOnCooldown() const;
	void SetIsOnCooldown(bool NewValue);

	int32 GetCurrentCharges() const;
	void SetCurrentCharges(int32 NewValue);

	int32 GetMaxRecharges() const;

	/** 충전이 여럿인지가 이 값에서 파생되므로 함께 갱신된다. */
	void SetMaxRecharges(int32 NewValue);

	bool GetHasMultipleCharges() const;
	void SetHasMultipleCharges(bool NewValue);

	bool GetCanActivate() const;
	void SetCanActivate(bool NewValue);

	bool GetCheckCost() const;
	void SetCheckCost(bool NewValue);

	float GetCostAmount() const;
	void SetCostAmount(float NewValue);

	UObject* GetIcon() const;
	void SetIcon(UObject* NewValue);

protected:
	//~ Begin UWxViewModel
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
	//~ End UWxViewModel

private:
	void HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleCostAttributeChanged(const FOnAttributeChangeData& Data);
	bool UpdateCooldownState(float DeltaTime);

	bool FlushActivationRefresh(float DeltaTime);

	void StartCooldownTicker();
	void StopCooldownTicker();

	/**
	 * 쿨다운 태그를 부여하는 활성 GE 하나를 찾아 소모된 충전 수(스택 수)를 반환하고, 다음 충전까지의 잔여·회복 시간을 낸다.
	 * 순정 조회 API 는 스택 수를 함께 주지 않는 데다 호출마다 배열을 새로 할당하므로, 매 프레임 도는 이 경로에서는 컨테이너를 직접 한 번만 훑는다.
	 */
	int32 QueryCooldownStacks(const UAbilitySystemComponent& ASC, float WorldTime, float& OutRemaining, float& OutDuration) const;

	void RefreshActivationState();

	/**
	 * 비용 GE 가 실제로 깎는 자원을 OutCostAttribute 로 내고 그 양을 반환한다. 깎는 자원이 없으면 0.
	 * 비용 GE 는 자원별 모디파이어를 모두 선언해 두고 값은 적용 시점에 계산하므로, 정의만 읽어서는 알 수 없어 스펙을 한 번 평가한다.
	 */
	float QueryCost(const UAbilitySystemComponent& ASC, const UGameplayAbility& Ability, FGameplayAttribute& OutCostAttribute) const;

	/** 비용 자원과 그 양을 정하고, 값이 바뀌면 발동 가능 판정을 다시 하도록 구독한다. */
	void BindCostAttributes(UAbilitySystemComponent& ASC, const UGameplayAbility& Ability);

	/** 어빌리티마다 다른 구독이라 어빌리티를 놓을 때마다 푼다. */
	void UnbindCostAttributes(UAbilitySystemComponent& ASC);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;

	/** 이 슬롯이 가리키는 어빌리티 에셋 태그. Asset Tags 가 이것을 모두 포함하는(HasAll) 어빌리티를 문다. */
	FGameplayTagContainer AbilityTags;

	/** 어빌리티의 쿨다운 GE 가 부여하는 태그. 비어 있으면 쿨다운이 없는 어빌리티다. */
	FGameplayTagContainer CachedCooldownTags;

	/** 비용 GE가 깎는 자원 어트리뷰트와 그 최대치. 값 변경 델리게이트 등록/해제용 */
	FGameplayAttribute CostAttribute;
	FGameplayAttribute CostMaxAttribute;

	FTSTicker::FDelegateHandle TickerHandle;

	/** 유효하면 이번 프레임의 재평가가 이미 예약돼 있다는 뜻이다. */
	FTSTicker::FDelegateHandle ActivationRefreshHandle;
};
