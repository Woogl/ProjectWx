// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CommonActionWidget.h"

#include "WxActionWidget.generated.h"

/**
 * 디자인 타임에도 InputActions(버튼의 TriggeringInputAction)로부터 인풋 아이콘을 미리 보여주는 액션 위젯.
 * 런타임 동작은 UCommonActionWidget 그대로다.
 */
UCLASS()
class WXUI_API UWxActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:
	virtual FSlateBrush GetIcon() const override;
};
