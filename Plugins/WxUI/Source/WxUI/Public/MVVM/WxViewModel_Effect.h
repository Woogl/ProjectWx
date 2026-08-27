// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Containers/Ticker.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Effect.generated.h"

class UWxEffectComponent_UIData;
class UAbilitySystemComponent;

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

	/** UIData 의 소프트 참조를 베이스가 비동기 로드해 세팅한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|Effect")
	TObjectPtr<UObject> Icon = nullptr;

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
	void HandleStackCountChanged(FActiveGameplayEffectHandle Handle, int32 NewStackCount, int32 PreviousStackCount);

	bool UpdateEffectState(float DeltaTime);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FActiveGameplayEffectHandle BoundHandle;
	FDelegateHandle StackChangeHandle;
	FTSTicker::FDelegateHandle TickerHandle;
};
