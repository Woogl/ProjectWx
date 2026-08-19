// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModel_Attribute.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;
class UMVVMView;
class UUserWidget;

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
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	float CurrentAttribute = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	float MaxAttribute = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	float AttributePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	bool IsAttributeEmpty = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Attribute")
	bool IsAttributeFull = false;

	float GetCurrentAttribute() const;
	void SetCurrentAttribute(float NewValue);

	float GetMaxAttribute() const;
	void SetMaxAttribute(float NewValue);

	float GetAttributePercent() const;
	void SetAttributePercent(float NewValue);

	bool GetIsAttributeEmpty() const;
	void SetIsAttributeEmpty(bool bNewValue);

	bool GetIsAttributeFull() const;
	void SetIsAttributeFull(bool bNewValue);

	FGameplayAttribute GetBoundAttribute() const;

	FGameplayAttribute GetBoundMaxAttribute() const;

private:
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleMaxAttributeChanged(const FOnAttributeChangeData& Data);
	void RecalculateAttributePercent();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FGameplayAttribute BoundAttribute;
	FGameplayAttribute BoundMaxAttribute;
};

/**
 * 위젯을 소유한 PlayerController 의 빙의 Pawn 에서 ASC 를 끌어와, 지정한 어트리뷰트 쌍의 UWxViewModel_Attribute 를 그 ASC 의 어빌리티시스템 VM 에서 얻는다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택하고, 위젯마다 표시할 어트리뷰트를 지정한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_Attribute : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayAttribute CurrentAttribute;

	/** 지정하지 않으면 CurrentAttribute 를 최대값으로 쓴다 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayAttribute MaxAttribute;
};
