// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractionWidgetInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWxInteractionWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * UWxInteractionWidgetComponent가 사용하는 프롬프트 위젯 계약.
 * 위젯 BP가 본 인터페이스를 구현하면, 컴포넌트가 InteractionText를 위젯에 전달한다.
 */
class WXWORLD_API IWxInteractionWidgetInterface
{
	GENERATED_BODY()

public:
	/** 프롬프트에 표시할 텍스트를 위젯에 전달한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wx")
	void SetInteractionText(const FText& InText);
};
