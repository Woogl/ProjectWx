// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_GameplayTag.generated.h"

class UAbilitySystemComponent;

/**
 * 게임플레이 태그 뷰모델.
 * ASC의 모든 태그 변경을 감지하여, 현재 보유 중인 태그 컨테이너를 UI 바인딩용으로 갱신한다.
 * 위젯 바인딩에서 UWxUIFunctionLibrary::HasGameplayTag 컨버전 함수를 사용하여 특정 태그 보유 여부를 판정한다.
 */
UCLASS()
class WXUI_API UWxViewModel_GameplayTag : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category = "Wx|GameplayTag")
	FGameplayTagContainer OwnedTags;

	FGameplayTagContainer GetOwnedTags() const;
	void SetOwnedTags(const FGameplayTagContainer& NewTags);

protected:
	virtual void Deinitialize() override;

private:
	void HandleGameplayTagChanged(const FGameplayTag Tag, int32 NewCount);

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
