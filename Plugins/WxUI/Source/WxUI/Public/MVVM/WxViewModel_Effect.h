// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Containers/Ticker.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Effect.generated.h"

class UAbilitySystemComponent;

/**
 * GameplayEffect 뷰모델.
 * 활성 GameplayEffect의 남은 시간, 스택 수, 아이콘을 UI에 제공한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC, Handle, EffectName, Icon)로 초기화
 *  2. 매 프레임 남은 시간, 스택 수를 갱신
 *  3. 이펙트 제거 시 타이머 중단, 프로퍼티 초기화
 */
UCLASS()
class WXUI_API UWxViewModel_Effect : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InHandle, const FText& InEffectName, UTexture2D* InIcon);

	FActiveGameplayEffectHandle GetBoundHandle() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	FText EffectName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	float TimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	float Duration = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	float TimeRemainingPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	int32 StackCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Wx|Effect")
	FGameplayTag EffectTag;

	FText GetEffectName() const;
	void SetEffectName(const FText& NewValue);

	float GetTimeRemaining() const;
	void SetTimeRemaining(float NewValue);

	float GetDuration() const;
	void SetDuration(float NewValue);

	float GetTimeRemainingPercent() const;
	void SetTimeRemainingPercent(float NewValue);

	int32 GetStackCount() const;
	void SetStackCount(int32 NewValue);

	UTexture2D* GetIcon() const;
	void SetIcon(UTexture2D* NewValue);
	
	FGameplayTag GetEffectTag() const;

protected:
	virtual void Deinitialize() override;

private:
	bool UpdateEffectState(float DeltaTime);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FActiveGameplayEffectHandle BoundHandle;
	float EffectEndTime = 0.f;
	float CachedDuration = 0.f;
	FTSTicker::FDelegateHandle TickerHandle;
};
