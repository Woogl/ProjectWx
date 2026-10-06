// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Attribute.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;

UCLASS()
class WXGAME_API UWxViewModel_Attribute : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Max 생략 시 Current를 최대값으로 사용한다. 생성 직후 한 번만 부른다. */
	void Initialize(UAbilitySystemComponent* InASC, FGameplayAttribute InAttribute, FGameplayAttribute InMaxAttribute);
	void Deinitialize();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Attribute")
	float AttributeAmount = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Attribute")
	float MaxAttributeAmount = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Attribute")
	float AttributePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Attribute")
	bool IsAttributeFull = false;

	FGameplayAttribute GetBoundAttribute() const;
	FGameplayAttribute GetBoundMaxAttribute() const;

private:
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleMaxAttributeChanged(const FOnAttributeChangeData& Data);
	void RefreshDerivedFields();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FGameplayAttribute BoundAttribute;
	FGameplayAttribute BoundMaxAttribute;
};
