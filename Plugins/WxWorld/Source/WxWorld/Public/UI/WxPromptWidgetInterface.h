// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxPromptWidgetInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWxPromptWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * UWxPromptWidgetComponent가 사용하는 프롬프트 위젯 계약.
 * 위젯 BP가 본 인터페이스를 구현하면, 컴포넌트가 PromptText를 위젯에 전달한다.
 */
class WXWORLD_API IWxPromptWidgetInterface
{
	GENERATED_BODY()

public:
	/** 프롬프트에 표시할 텍스트를 위젯에 전달한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wx")
	void SetPromptText(const FText& InText);
};
