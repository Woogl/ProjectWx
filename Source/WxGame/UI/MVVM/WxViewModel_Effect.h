// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/TimerHandle.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Effect.generated.h"

struct FStreamableHandle;
class UAbilitySystemComponent;
class UGameplayEffect;
class UWxViewModel_Effect;

/** 표시할 효과이면 필드를 채우고 true를 반환한다. */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FWxConfigureEffectViewModel, UWxViewModel_Effect&, const UGameplayEffect&);

UCLASS()
class WXGAME_API UWxViewModel_Effect : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InHandle, const FWxConfigureEffectViewModel& InConfigurePresentation);
	void SetPresentation(const FText& InTitle, const FText& InDescription, const TSoftObjectPtr<UObject>& InIcon);

	/** 효과가 걷히면 목록 VM 이 부른다 — 이 VM 을 아직 붙든 위젯에 빈 값을 통지한다. */
	void Deinitialize();

	FActiveGameplayEffectHandle GetBoundHandle() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	FText Title;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	FText Description;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	float TimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	float Duration = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	float TimeRemainingPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	int32 StackCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	bool IsStackCountAboveOne = false;

	/** UIData 의 소프트 참조를 비동기 로드해 세팅한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Effect")
	TObjectPtr<UObject> Icon = nullptr;

private:
	/** 스택이 여럿인지가 이 값에서 파생되므로 함께 갱신된다. */
	void SetStackCount(int32 NewValue);

	void SetIcon(const TSoftObjectPtr<UObject>& InIcon);

	void HandleStackCountChanged(FActiveGameplayEffectHandle Handle, int32 NewStackCount, int32 PreviousStackCount);

	bool UpdateEffectState();
	void HandleTimeRemainingTimer();

	void StartTimeRemainingTimer();
	void StopTimeRemainingTimer();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	FActiveGameplayEffectHandle BoundHandle;
	FDelegateHandle StackChangeHandle;
	FTimerHandle TimeRemainingTimerHandle;
	TSharedPtr<FStreamableHandle> IconHandle;
};
