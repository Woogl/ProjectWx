// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"
#include "WxEffectComponent_UIData.generated.h"

/**
 * GameplayEffect UI 데이터 컴포넌트.
 * 효과 이름과 아이콘을 에디터에서 설정하여 UI에 표시할 수 있도록 한다.
 */
UCLASS(DisplayName = "UI Data")
class UWxEffectComponent_UIData : public UGameplayEffectUIData
{
	GENERATED_BODY()

public:
	UWxEffectComponent_UIData();
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MultiLine = "true"))
	FText Description;
};
