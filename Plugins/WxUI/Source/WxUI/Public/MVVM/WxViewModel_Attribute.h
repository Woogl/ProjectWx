// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Attribute.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;

/**
 * 범용 어트리뷰트 뷰모델.
 * ASC의 임의의 어트리뷰트 쌍(현재값, 최대값)의 변경을 감지하여 UI 바인딩용 프로퍼티를 갱신한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Attribute : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC, FGameplayAttribute InAttribute, FGameplayAttribute InMaxAttribute);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	float CurrentAttribute = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	float MaxAttribute = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	float AttributePercent = 0.f;

	float GetCurrentAttribute() const;
	void SetCurrentAttribute(float NewValue);

	float GetMaxAttribute() const;
	void SetMaxAttribute(float NewValue);

	float GetAttributePercent() const;
	void SetAttributePercent(float NewValue);

protected:
	virtual void Deinitialize() override;

private:
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleMaxAttributeChanged(const FOnAttributeChangeData& Data);
	void RecalculateAttributePercent();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FGameplayAttribute BoundAttribute;
	FGameplayAttribute BoundMaxAttribute;
};
