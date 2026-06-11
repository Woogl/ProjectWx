// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxAbilityComponent.h"
#include "WxAbilityComponent_UIData.generated.h"

class UTexture2D;

/**
 * 어빌리티 UI 표시용 데이터 컴포넌트.
 * UI 데이터가 필요한 어빌리티 BP 의 디테일 패널(Wx)에서 컴포넌트로 추가해 사용한다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, DisplayName = "Wx Ability UI Data")
class WXUI_API UWxAbilityComponent_UIData : public UWxAbilityComponent
{
	GENERATED_BODY()

public:
	/** 어빌리티 아이콘. UI에서 표시. 비동기 로드 권장. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;
};
