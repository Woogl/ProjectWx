// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxIndicatorWidget.generated.h"

/**
 * 화면 인디케이터로 띄울 수 있는 위젯이라는 표식.
 *
 * 값은 인디케이터 액터가 넣어 주는 UWxViewModel_Indicator 로만 들어오므로 구현할 함수가 없다 — 뷰모델 소스 없는 위젯이 노드에 꽂히지 않도록 픽커를 좁히는 것이 이 인터페이스의 전부다.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UWxIndicatorWidget : public UInterface
{
	GENERATED_BODY()
};

class WXUI_API IWxIndicatorWidget
{
	GENERATED_BODY()
};
