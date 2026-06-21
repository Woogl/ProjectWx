// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

/** 엘리베이터 상태(끝점 + 문). Start=거리 0, End=SplineLength. Closed 만 문 닫힘, 나머지는 문 열림. */
UENUM()
enum class EWxElevatorState : uint8
{
	/** Start(거리 0) + 문 닫힘. 초기 상태. 콘솔 호출 시 문이 열린다(같은 층이면 이동 없이 AtStart 로, 다른 층이면 이동 후). */
	Closed,
	/** Start(거리 0) + 문 열림. */
	AtStart,
	/** End(SplineLength) + 문 열림. */
	AtEnd
};

/**
 * 엘리베이터.
 * SplineComponent(닫힌 루프)가 정의하는 경로의 두 끝점(Start/End) 사이를 플랫폼이 왕복한다.
 *
 * 도어와 동일한 결: 권위 State(Closed/AtStart/AtEnd)는 인터랙션 시 즉시 최종값으로 확정한다.
 * 이동/문 개폐는 권위와 무관한 순수 비주얼이라 "도착"이라는 권위 사건이 없다.
 *   - 플랫폼 이동: StateTree 의 Wx Component Spline Move 가 닫힌 루프 위를 라이브 전이마다 한 세그먼트 전진(Start↔End). 닫힌 루프라 양방향이 모두 정방향 한 세그먼트다.
 *   - 문 개폐·인터랙션 토글: 각 상태의 Wx Component Move / Wx Enable Interaction(이동할 메시·오프셋·시퀀스는 ST_Elevator 에셋에서 author).
 *
 * Closed 와 AtStart 는 같은 위치(Start)다. Closed→AtStart 는 "같은 층, 이동 없이 문만 열기"이므로,
 * ST_Elevator 는 Start/End 끝점별 부모 상태가 Spline Move 를 들고 그 아래 문 상태(Closed/AtStart, AtEnd)를 자식으로 두어,
 * 같은 끝점 안의 전이(Closed↔AtStart)에선 부모(=Spline Move)가 재진입하지 않아 플랫폼이 전진하지 않게 author 한다.
 *
 * 전이는 ST_Elevator 의 Enum Compare 전이 조건(State 변화 감지)이 구동한다(이벤트 태그 없음, 서버/클라 동일).
 * 인터랙션(서버)이 현재 State 를 보고 다음 State 를 직접 확정한다:
 *   - CallConsoleA → AtStart, CallConsoleB → AtEnd (동일값이면 노옵). Platform → 반대 끝점 토글(문 열린 정지 상태에서만).
 *
 * 위치 정보:
 *   플랫폼 위치는 초기/복원/라이브 전부 Spline Move 가 전담한다(C++ 스냅 없음). 각 끝점 부모 상태의 Spline Move 가 TargetPointIndex 로 자기 끝점을 직접 가리키므로,
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> SplineComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PlatformRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Component Move 가 Context 액터의 컴포넌트로 바인딩하기 위한 노출(State 의 Enum Compare 바인딩과 동일 패턴).
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

	/** 권위 측에서 State 를 전환한다. 동일값/비권위면 노옵. 라이브 이동은 StateTree 가 추종하므로 ApplyState 를 부르지 않는다(위치는 Spline Move 가 전담). */
	void SetElevatorState(EWxElevatorState NewState);

	/**
	 * 엘리베이터 권위/영속 상태(끝점 + 문). 인터랙션 시 즉시 최종값으로 확정된다. 초기값은 Closed(Start, 문 닫힘).
	 * 클라는 복제된 State 를 ST_Elevator 의 Enum Compare 전이가 폴링해 추종하므로 RepNotify 불필요.
	 * VisibleAnywhere + AllowPrivateAccess 는 StateTree 의 Enum Compare 조건(enter/전이)이 바인딩하기 위한 노출이다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxElevatorState State = EWxElevatorState::Closed;
};
