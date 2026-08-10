// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CommonActionWidget.h"

#include "WxActionWidget.generated.h"

/**
 * 디자인 타임에도 버튼이 지정한 인풋 액션(EnhancedInput·DataTable 양쪽)에서 아이콘을 미리 보여주는 액션 위젯.
 * 런타임 동작은 UCommonActionWidget 그대로다.
 */
UCLASS()
class WXUI_API UWxActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:
	virtual FSlateBrush GetIcon() const override;
};
