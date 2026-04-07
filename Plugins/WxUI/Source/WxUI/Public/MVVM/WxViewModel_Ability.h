// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Ability.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayEffectSpec;

/**
 * 어빌리티 쿨다운 뷰모델.
 * ASC의 쿨다운 태그 변경을 감지하여 쿨다운 상태를 UI에 제공한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC, Ability)로 초기화 (Ability CDO 허용)
 *  2. 쿨다운 태그 부여 시 타이머로 매 프레임 남은 시간/남은 충전 수 갱신
 *  3. 쿨다운 만료 시 타이머 중단, 프로퍼티 초기화
 */
UCLASS()
class WXUI_API UWxViewModel_Ability : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * @param InASC          소유 ASC
	 * @param InAbility      어빌리티 CDO
	 * @param InMaxCharges   최대 충전 수. 1이면 단일 쿨다운, 2 이상이면 충전 시스템.
	 *                       호출자가 어빌리티 클래스에 맞는 값을 전달해야 한다.
	 */
	void Initialize(UAbilitySystemComponent* InASC, const UGameplayAbility* InAbility, int32 InMaxCharges = 1);

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
	int32 MaxCharges = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	bool HasMultipleCharges = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Ability")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Ability")
	FGameplayTag AbilityTag;

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

	int32 GetMaxCharges() const;
	void SetMaxCharges(int32 NewValue);

	bool GetHasMultipleCharges() const;
	void SetHasMultipleCharges(bool NewValue);

	UTexture2D* GetIcon() const;
	void SetIcon(UTexture2D* NewValue);

	FGameplayTag GetAbilityTag() const;

protected:
	virtual void Deinitialize() override;

private:
	void HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);
	bool UpdateCooldownState(float DeltaTime);

	FGameplayTag GetCooldownTag() const;
	int32 GetConsumedCharges() const;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;
	FTSTicker::FDelegateHandle TickerHandle;
};
