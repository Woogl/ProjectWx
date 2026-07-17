// Copyright Woogle. All Rights Reserved.


#include "Widget/WxActionWidget.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputTypeEnum.h"
#include "CommonUITypes.h"

FSlateBrush UWxActionWidget::GetIcon() const
{
	// 디자인 타임엔 엔진이 DesignTimeKey만 읽으므로, InputActions(TriggeringInputAction)에서 직접 아이콘을 해석한다.
	if (IsDesignTime())
	{
		if (const FCommonInputActionDataBase* InputActionData = GetInputActionData())
		{
			ECommonInputType InputType;
			FName GamepadName;
			FCommonInputBase::GetCurrentPlatformDefaults(InputType, GamepadName);

			const FKey Key = InputActionData->GetInputTypeInfo(InputType, GamepadName).GetKey();

			FSlateBrush Brush;
			if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(Brush, Key, InputType, GamepadName))
			{
				return Brush;
			}
		}
	}

	return Super::GetIcon();
}
