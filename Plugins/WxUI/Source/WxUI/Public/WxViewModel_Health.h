// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxViewModel.h"
#include "WxViewModel_Health.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;

/**
 * 체력 뷰모델.
 * ASC의 HP, MaxHP 어트리뷰트 변경을 감지하여 UI 바인딩용 프로퍼티를 갱신한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Health : public UWxViewModel
{
	GENERATED_BODY()

public:
	void InitializeWithASC(UAbilitySystemComponent* InASC, FName InHPName = TEXT("HP"), FName InMaxHPName = TEXT("MaxHP"));
	void DeinitializeFromASC();

	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Health")
	float CurrentHP = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Health")
	float MaxHP = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Health")
	float HealthPercent = 0.f;

	float GetCurrentHP() const;
	void SetCurrentHP(float NewValue);

	float GetMaxHP() const;
	void SetMaxHP(float NewValue);

	float GetHealthPercent() const;
	void SetHealthPercent(float NewValue);

private:
	void HandleHPChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHPChanged(const FOnAttributeChangeData& Data);
	void RecalculateHealthPercent();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FName HPName;
	FName MaxHPName;
};
