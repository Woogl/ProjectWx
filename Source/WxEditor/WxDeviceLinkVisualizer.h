// Copyright Woogle. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"

/**
 * 선택 중인 장치(AWxDevice)의 배선(LinkedDevices)을 시안 선으로 잇는다. 배치 저작 편의 전용이다.
 *
 * 언제·무엇에 그릴지는 전부 엔진이 정한다 — 선택이 바뀔 때 선택된 액터의 컴포넌트에서 이 비주얼라이저를 찾아 두고 뷰포트 드로우마다 부른다.
 * 그래서 선택 추적도 편집 이벤트 구독도 없고, 끝점을 매번 새로 읽으므로 드래그 도중에도 선이 따라온다.
 * 여러 장치를 함께 선택하면 각자의 배선이 함께 나온다.
 *
 * 등록 키는 엔진이 비워 둔 UStateTreeComponent 다 — 배선은 액터가 들고 있는데 비주얼라이저는 컴포넌트 클래스로만 등록되고, 장치의 컴포넌트는 WxWorld Private 이라 여기서 잡을 수 없다.
 * 그 자리에 등록하면 클래스 사슬을 거스르는 엔진 탐색이 장치의 파생 컴포넌트까지 덮으므로, 장치가 아닌 오너는 그리기 직전에 거른다.
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
