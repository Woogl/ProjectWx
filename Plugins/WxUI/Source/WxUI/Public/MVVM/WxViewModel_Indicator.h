// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"

#include "WxViewModel_Indicator.generated.h"

/**
 * 위젯은 인디케이터 액터의 위젯 컴포넌트가 만들고, 이 뷰모델도 그 액터가 만들어 묶는다.
 * 화면 좌표는 액터가 스스로 옮겨 다니므로 여기 없다 — 위젯이 자기 자리에서 알 수 없는 값만 받는다.
 */
UCLASS()
class WXUI_API UWxViewModel_Indicator : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 값이 달라졌을 때만 통지한다. */
	void SetProjection(float InDistanceMeters, bool bInClamped);

	/** 카메라에서 대상까지의 거리 (m). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Indicator")
	float CameraDistance = 0.f;

	/** 대상이 화면 밖이라 가장자리에 붙었는지. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Indicator")
	bool bClamped = false;
};
