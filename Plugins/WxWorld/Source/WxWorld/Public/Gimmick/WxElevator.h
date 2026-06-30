// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 엘리베이터.
 * SplineComponent(열린 경로)가 정의하는 두 끝점(Start/End) 사이를 플랫폼이 왕복한다.
 *
 * 도어와 동일한 결: 권위 State(Closed/AtStart/AtEnd)는 인터랙션 시 즉시 최종값으로 확정한다.
 * 이동/문 개폐는 권위와 무관한 순수 비주얼이라 "도착"이라는 권위 사건이 없다.
 *   - 플랫폼 이동: StateTree 의 Wx Component Spline Move 가 두 끝점 사이 단일 세그먼트를 라이브 전이마다 주파(Start↔End). 가장 가까운 포인트에서 목표 끝점으로 정/역방향 슬라이드한다.
 *   - 문 개폐·인터랙션 토글: 각 상태의 Wx Component Move / Wx Enable Interaction(이동할 메시·오프셋·시퀀스는 ST_Elevator 에셋에서 author).
 *
 * ST_Elevator 는 권위 State 값마다 최상위 leaf 하나(Closed/AtStart/AtEnd, Required Event to Enter = 그 State 태그)를 두고, 각 leaf 안에서 "문 닫기 → 플랫폼 이동 → 문 열기"를 자식 상태 시퀀스로 choreography 한다(각 단계 완료 시 다음 단계로).
 * mover 는 현재 위치에서 목표로 슬라이드하고 이미 목표면 즉시 완료하므로(self-anchoring), 같은 층 전이(Closed↔AtStart)는 닫기·이동 단계가 즉시 collapse 되고 문 개폐만 실제로 보인다.
 *
 * 전이는 State 가 바뀌면 베이스가 보내는 그 State 태그 이벤트가 구동한다(각 leaf 의 Required Event to Enter, 서버/클라 동일).
 * 인터랙션(서버)이 현재 State 를 보고 다음 State 를 직접 확정한다:
 *   - CallConsoleA → AtStart, CallConsoleB → AtEnd (동일값이면 노옵). Platform → 반대 끝점 토글(문 열린 정지 상태에서만).
 *
 * 위치 정보:
 *   플랫폼 위치는 초기/복원/라이브 전부 Spline Move 가 전담한다(C++ 스냅 없음). 각 leaf 의 이동 단계 Spline Move 가 TargetPointIndex 로 자기 끝점을 직접 가리키므로,
 *   StateTree 시작·스트리밍 복원 시에도 mover 가 복제/복원된 State 의 끝점으로 스냅한다(과거의 BeginPlay 1회 C++ 스냅이 불필요해짐).
 *
 * 인터랙션 영역은 셋:
 *  - PlatformInteraction: 플랫폼 위에서 상호작용하면 반대 끝점으로 이동
 *  - CallConsoleAInteraction: 플랫폼을 스플라인 시작점(거리 0)으로 호출
 *  - CallConsoleBInteraction: 플랫폼을 스플라인 끝점(SplineLength)으로 호출
 */
UCLASS(Abstract)
class WXWORLD_API AWxElevator : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxElevator();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> SplineComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PlatformRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Component Move 가 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorRight;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> PlatformInteraction;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> CallConsoleA;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> CallConsoleAInteraction;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> CallConsoleB;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> CallConsoleBInteraction;

private:
	UFUNCTION()
	void HandlePlatformInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleCallConsoleAInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleCallConsoleBInteracted(AActor* InteractingActor);
};
