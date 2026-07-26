// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WxIndicatorWidget.generated.h"

/**
 * 화면 인디케이터 위젯의 베이스.
 * 위젯은 자기가 화면 어디에 놓이는지 알지 않는다 — 배치는 캔버스가 하고, 위젯은 무엇을 그릴지만 정한다.
 * WBP 에서 OnIndicatorRefreshed 를 구현해 거리 문구나 화면 밖일 때의 외형을 저작한다.
 */
UCLASS(Abstract)
class WXUI_API UWxIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 캔버스가 투영 결과를 알린다. DistanceMeters 는 카메라에서 대상까지의 거리, bClamped 는 대상이 화면 밖이라 가장자리에 붙었는지다.
	 * 값이 유의미하게 바뀔 때만 호출된다(매 프레임 호출이 아니다).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wx|Indicator")
	void OnIndicatorRefreshed(float DistanceMeters, bool bClamped);
};
