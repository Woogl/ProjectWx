// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
 * 어빌리티 쿨다운 뷰모델.
 * 어빌리티의 GetCooldownGameplayEffect()를 기준으로 쿨다운/충전 상태를 UI에 제공한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC, Ability)로 초기화. 어빌리티 CDO에서 CooldownGE 클래스와
 *     StackLimitCount(=MaxCharges)를 자동으로 읽어온다.
 *  2. 쿨다운 GE 적용 시 타이머로 매 프레임 남은 시간/남은 충전 수 갱신
 *  3. 쿨다운 만료 시 타이머 중단, 프로퍼티 초기화
 *
 * 동일 GE 클래스를 여러 어빌리티가 공유하는 경우, 소스 어빌리티 CDO로 구분한다.
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

	int32 GetConsumedCharges() const;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	TWeakObjectPtr<const UGameplayAbility> CachedAbility;
	TSubclassOf<UGameplayEffect> CachedCooldownClass;
	FTSTicker::FDelegateHandle TickerHandle;
};
