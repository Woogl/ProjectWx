// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Ability.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * 어빌리티 쿨다운 뷰모델.
 * ASC의 쿨다운 태그 변경을 감지하여 쿨다운 상태를 UI에 제공한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC, Ability)로 초기화 (Ability CDO 허용)
 *  2. 쿨다운 태그 부여 시 타이머로 매 프레임 남은 시간 갱신
 *  3. 쿨다운 만료 시 타이머 중단, 프로퍼티 초기화
 */
UCLASS()
class WXUI_API UWxViewModel_Ability : public UWxViewModel
{
	GENERATED_BODY()

public:
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
	TObjectPtr<UTexture2D> Icon = nullptr;

	float GetCooldownRemaining() const;
	void SetCooldownRemaining(float NewValue);

	float GetCooldownDuration() const;
	void SetCooldownDuration(float NewValue);

	float GetCooldownPercent() const;
	void SetCooldownPercent(float NewValue);

	bool GetIsOnCooldown() const;
	void SetIsOnCooldown(bool NewValue);

	UTexture2D* GetIcon() const;
	void SetIcon(UTexture2D* NewValue);

protected:
	virtual void Deinitialize() override;

private:
	void HandleCooldownTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	bool UpdateCooldownState(float DeltaTime);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FGameplayTag BoundCooldownTag;
	float CooldownEndTime = 0.f;
	float CachedCooldownDuration = 0.f;
	FTSTicker::FDelegateHandle TickerHandle;
};
