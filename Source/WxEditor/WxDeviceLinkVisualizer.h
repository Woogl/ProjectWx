// Copyright Woogle. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"

/**
 * 선택 중인 장치(AWxDevice)의 배선(LinkedDevices)을 선으로 잇는다. 배치 저작 편의 전용이다.
 *
 * 그릴 대상·시점은 엔진이 정하므로 선택 추적도 편집 이벤트 구독도 없다.
 *
 * 등록 키는 엔진이 비워 둔 UStateTreeComponent 다 — 배선은 액터가 들고 있는데 비주얼라이저는 컴포넌트 클래스로만 등록되고, 장치의 UWxDeviceStateTreeComponent 는 모듈 밖으로 export 되지 않아 여기서 잡을 수 없다.
 *
 * 루트가 아닌 컴포넌트에 걸려 있어 레벨 에디터의 Show Selection Subcomponents(기본 켜짐)를 끄면 그려지지 않는다.
 */
class FWxDeviceLinkVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;

private:
	FLinearColor LinkColor = FLinearColor(0.0f, 1.0f, 1.0f);

	/** 0 은 거리와 무관한 1픽셀 선이다 — 멀리서도 배선이 보인다. */
	float LinkThickness = 0.0f;
};
