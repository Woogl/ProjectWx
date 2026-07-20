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
struct FGameplayEffectSpec;

/**
 * 어빌리티 쿨다운/발동 가능 여부 뷰모델.
 * 어빌리티의 GetCooldownGameplayEffect()를 기준으로 쿨다운/충전 상태를 UI에 제공한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC, Ability)로 초기화.
 *     어빌리티 CDO에서 CooldownGE 클래스와 StackLimitCount(=MaxRecharges)를 자동으로 읽어온다.
 *  2. 쿨다운 GE 적용 시 타이머로 매 프레임 남은 시간/남은 충전 수 갱신
 *  3. 쿨다운 만료 시 타이머 중단, 프로퍼티 초기화
 *
 * 동일 GE 클래스를 여러 어빌리티가 공유하는 경우, 소스 어빌리티 CDO로 구분한다.
 *
 * CanActivate는 엔진 CanActivateAbility(비용/쿨다운/태그 요건 종합)로 판정하며, ASC 태그 변경/비용 어트리뷰트 변경/쿨다운 진행 시점에 재평가된다.
 * CheckCost는 같은 시점에 동명의 엔진 함수(비용 검사)만으로 판정한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Ability : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * @param InASC      소유 ASC
	 * @param InAbility  어빌리티 CDO. GetCooldownGameplayEffect()에서 충전 정보 추출.
	 */
	void Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility);
	virtual void Deinitialize() override;

	/** 바인딩된 어빌리티의 발동을 시도한다. 위젯 OnClicked 등 MVVM 이벤트 바인딩의 대상으로 사용한다. 비용/쿨다운/태그 요건은 엔진 TryActivateAbility가 판정한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Ability")
	bool TryActivateAbility();

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

	/** 발동 가능 여부. 비용/쿨다운/태그 요건을 엔진 CanActivateAbility로 종합 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool CanActivate = false;

	/** 비용 지불 가능 여부. 쿨다운/태그와 무관하게 엔진 CheckCost만으로 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool CheckCost = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** 바인딩된 어빌리티의 Asset Tags. 어빌리티 식별/매칭에 사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Ability")
	FGameplayTagContainer AbilityTags;

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

	UTexture2D* GetIcon() const;
	void SetIcon(UTexture2D* NewValue);

private:
	void HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleCostAttributeChanged(const FOnAttributeChangeData& Data);
	bool UpdateCooldownState(float DeltaTime);

	/** 부여된 스펙을 찾아 발동 가능 여부(CanActivateAbility)와 비용 지불 가능 여부(CheckCost)를 재평가한다 */
	void RefreshActivationState();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;
	TSubclassOf<UGameplayEffect> CachedCooldownClass;

	/** 비용 GE가 수정하는 어트리뷰트. 값 변경 델리게이트 등록/해제용 */
	TArray<FGameplayAttribute> CostAttributes;

	FTSTicker::FDelegateHandle TickerHandle;
};
