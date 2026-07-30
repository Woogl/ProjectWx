// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStaticMeshComponent;

/**
 * 엘리베이터.
 * SplineComponent(열린 경로)가 정의하는 두 끝점(Start/End) 사이를 플랫폼이 왕복한다.
 *
 * 도어와 동일한 결: 상태(Closed/AtStart/AtEnd)는 상호작용 즉시 최종값으로 갈아탄다.
 * 이동/문 개폐는 그 상태를 따라가는 순수 비주얼이라 "도착"이라는 권위 사건이 없다.
 *   - 플랫폼 이동: StateTree 의 Component Spline Move 가 두 끝점 사이 단일 세그먼트를 라이브 전이마다 주파(Start↔End).
 *     가장 가까운 포인트에서 목표 끝점으로 정/역방향 슬라이드한다.
 *   - 문 개폐·인터랙션 토글: 각 상태의 Component Move / Enable Interaction(이동할 메시·오프셋·시퀀스는 ST_Elevator 에셋에서 author).
 *
 * ST_Elevator 는 상태값마다 최상위 leaf 하나(Closed/AtStart/AtEnd, 그 leaf 의 Tag 가 곧 저장 값)를 두고, 각 leaf 안에서 "문 닫기 → 플랫폼 이동 → 문 열기"를 자식 상태 시퀀스로 choreography 한다(각 단계 완료 시 다음 단계로).
 * mover 는 현재 위치에서 목표로 슬라이드하고 이미 목표면 즉시 완료하므로(self-anchoring), 같은 층 전이(Closed↔AtStart)는 닫기·이동 단계가 즉시 collapse 되고 문 개폐만 실제로 보인다.
 *
 * 전이는 상호작용 이벤트를 받는 각 leaf 의 전이가 구동한다. 세 영역이 모두 공용 태그(StateTree.Interact)로 발동하고, 갈 곳이 갈리는 상태에서만 전이 조건이 페이로드의 Source 를 대상 메시와 비교해 가른다(전 피어 동일):
 *   - Closed 는 두 콘솔이 서로 다른 끝점을 부르므로 전이마다 Object Equals 조건을 둔다(Source==CallConsoleA → AtStart, Source==CallConsoleB → AtEnd).
 *   - AtStart/AtEnd 는 자기 층 콘솔을 Enable Interaction 에서 꺼두어 남은 영역(반대편 콘솔·Platform)이 전부 같은 목적지라, 조건 없는 단일 전이로 반대 끝점에 간다.
 *
 * 위치 정보:
 *   플랫폼 위치는 초기/복원/라이브 전부 Spline Move 가 전담한다(C++ 스냅 없음).
 *   각 leaf 의 이동 단계 Spline Move 가 TargetPointIndex 로 자기 끝점을 직접 가리키므로, 저장된 상태에서 트리를 열기만 해도 mover 가 그 끝점으로 스냅한다.
 *
 * 인터랙션 영역은 셋(각 메시가 곧 영역이다):
 *  - PlatformMesh: 플랫폼 위에서 상호작용하면 반대 끝점으로 이동
 *  - CallConsoleA: 플랫폼을 스플라인 시작점(거리 0)으로 호출
 *  - CallConsoleB: 플랫폼을 스플라인 끝점(SplineLength)으로 호출
 *
 * 프롬프트는 영역마다 갈리며, 세 영역 모두 ST_Elevator 의 각 상태 Enable Interaction Prompt 에서 author 한다(층에 따라 문구가 달라지는 것도 같은 자리에서 해결된다).
 */
UCLASS(Abstract)
class WXWORLD_API AWxElevator : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxElevator();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> SplineComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PlatformRoot;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Enable Interaction 이 토글 대상으로, 전이 조건이 상호작용 영역 비교 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Component Move 가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorRight;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Enable Interaction 이 토글 대상으로, 전이 조건이 상호작용 영역 비교 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CallConsoleA;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Enable Interaction 이 토글 대상으로, 전이 조건이 상호작용 영역 비교 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CallConsoleB;
};
