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
 * CanActivate·CheckCost 는 ASC 태그 변경/비용 어트리뷰트 변경/쿨다운 진행 시점에 재평가된다.
 */
UCLASS()
class WXUI_API UWxViewModel_Ability : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** @param InAbility 어빌리티 CDO */
	void Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility);
	virtual void Deinitialize() override;

	/**
	 * 위젯 OnClicked 등 MVVM 이벤트 바인딩의 대상으로 사용한다.
	 * 비용/쿨다운/태그 요건은 엔진 TryActivateAbility가 판정한다.
	 */
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

	/** 비용/쿨다운/태그 요건을 엔진 CanActivateAbility로 종합 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool CanActivate = false;

	/** 쿨다운/태그와 무관하게 엔진 CheckCost만으로 판정한다 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool CheckCost = false;

	/** 텍스처 또는 머터리얼이며, 소프트 참조는 SetIconSoft 가 비동기 로드한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	TObjectPtr<UObject> Icon = nullptr;

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
	 * 이 VM 은 UMG 바인딩 최초 평가 시 지연 생성되므로(메뉴 최초 오픈·HUD 재생성) 쿨다운 도중에 태어날 수 있다.
	 * 그때는 GE 적용 통지를 받을 기회가 없어 다음 발동까지 "충전 만땅"으로 잘못 표시된다.
	 */
	void SeedActiveCooldown();

	/** 부여된 스펙을 찾아 발동 가능 여부(CanActivateAbility)와 비용 지불 가능 여부(CheckCost)를 재평가한다 */
	void RefreshActivationState();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;
	TSubclassOf<UGameplayEffect> CachedCooldownClass;

	/** 비용 GE가 수정하는 어트리뷰트. 값 변경 델리게이트 등록/해제용 */
	TArray<FGameplayAttribute> CostAttributes;

	FTSTicker::FDelegateHandle TickerHandle;
};
