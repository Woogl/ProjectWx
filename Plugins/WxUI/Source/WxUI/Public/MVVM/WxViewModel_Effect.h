// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Containers/Ticker.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Effect.generated.h"

class UWxEffectComponent_UIData;
class UAbilitySystemComponent;

/**
 * GameplayEffect 뷰모델.
 * 활성 GameplayEffect의 남은 시간, 스택 수, 아이콘을 UI에 제공한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC, Handle, UIData)로 초기화
 *  2. 매 프레임 남은 시간, 스택 수를 갱신
 *  3. 이펙트 제거 시 타이머 중단, 프로퍼티 초기화
 */
UCLASS()
class WXUI_API UWxViewModel_Effect : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InHandle, const UWxEffectComponent_UIData* InUIData);
	virtual void Deinitialize() override;

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
	bool IsStackCountAboveOne = false;

	/** UI 표시 아이콘. UIData 의 소프트 참조를 베이스가 비동기 로드해 세팅하며, 텍스처와 머터리얼 양쪽이 올 수 있다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	TObjectPtr<UObject> Icon = nullptr;
	
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
	
	bool GetIsStackCountAboveOne() const;
	void SetIsStackCountAboveOne(bool bNewValue);

	UObject* GetIcon() const;
	void SetIcon(UObject* NewValue);

protected:
	//~ Begin UWxViewModel
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
	//~ End UWxViewModel

private:
	bool UpdateEffectState(float DeltaTime);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FActiveGameplayEffectHandle BoundHandle;
	float EffectEndTime = 0.f;
	float CachedDuration = 0.f;
	FTSTicker::FDelegateHandle TickerHandle;
};
