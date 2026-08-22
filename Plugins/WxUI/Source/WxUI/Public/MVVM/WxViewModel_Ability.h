// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Ability.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class UTexture2D;
struct FGameplayEffectSpec;

/**
 * 어빌리티 쿨다운/발동 가능 여부 뷰모델.
 * 쿨다운/충전 상태는 어빌리티의 GetCooldownGameplayEffect() 기준이며, MaxRecharges 는 그 GE 의 StackLimitCount 에서 읽는다.
 * 쿨다운 GE 가 적용되면 티커로 매 프레임 남은 시간·충전 수를 갱신하고, 만료되면 티커를 멈추고 프로퍼티를 초기화한다.
 *
 * 동일 GE 클래스를 여러 어빌리티가 공유하는 경우, 소스 어빌리티 CDO로 구분한다.
 *
 * CanActivate·CheckCost 는 ASC 태그 변경/비용 어트리뷰트 변경/쿨다운 적용·충전 수 변화 시점에 재평가된다.
 *
 * 소모량은 초기화 때 비용 GE 를 한 번 평가해 정한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Ability : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** @param InAbility 어빌리티 CDO */
	void Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility);
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Wx|Ability")
	bool TryActivateAbility();

	/** 아이콘처럼 UI 플러그인이 읽지 못하는 표시 데이터를 게임 모듈이 채울 때 쓴다. */
	const UGameplayAbility* GetBoundAbility() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	float CooldownRemaining = 0.f;

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

	/** 텍스처 또는 머터리얼이며, 소프트 참조는 SetIconSoft 가 비동기 로드한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	TObjectPtr<UObject> Icon = nullptr;

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

	/**
	 * 로드 완료 시 Icon(하드)을 세팅해 바인딩을 발화한다.
	 * null이면 즉시 Icon을 비운다.
	 */
	void SetIconSoft(const TSoftObjectPtr<UObject>& InIcon);

protected:
	//~ Begin UWxViewModel
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
	//~ End UWxViewModel

private:
	void HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleCostAttributeChanged(const FOnAttributeChangeData& Data);
	bool UpdateCooldownState(float DeltaTime);

	/**
	 * 초기화 시점에 이미 돌고 있는 쿨다운을 1회 스캔해 반영한다.
	 * 이 VM 은 UMG 바인딩 최초 평가 시 지연 생성되므로 쿨다운 도중에 태어나면 GE 적용 통지를 놓쳐 "충전 만땅"으로 잘못 표시된다.
	 */
	void SeedActiveCooldown();

	void EnsureCooldownTicker();

	/**
	 * 이 어빌리티가 건 활성 쿨다운 GE 를 훑어 소모된 충전 수를 반환하고, 가장 먼저 만료될 GE 의 잔여·지속시간을 낸다.
	 * FGameplayEffectQuery 경로는 핸들 배열을 새로 할당한 뒤 핸들마다 컨테이너를 다시 선형 탐색하므로, 매 프레임 도는 이 경로에서는 컨테이너를 직접 한 번만 훑는다.
	 */
	int32 QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float WorldTime, float& OutNextRemaining, float& OutNextDuration) const;

	void RefreshActivationState();

	/**
	 * 비용 GE 가 실제로 깎는 자원과 그 양을 정한다.
	 * 비용 GE 는 자원별 모디파이어를 모두 선언해 두고 값은 적용 시점에 계산하므로, 정의만 읽어서는 알 수 없어 스펙을 한 번 평가한다.
	 */
	void GetCost(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;
	TSubclassOf<UGameplayEffect> CachedCooldownClass;

	/** 비용 GE가 깎는 자원 어트리뷰트와 그 최대치. 값 변경 델리게이트 등록/해제용 */
	FGameplayAttribute CostAttribute;
	FGameplayAttribute CostMaxAttribute;

	FTSTicker::FDelegateHandle TickerHandle;
};
