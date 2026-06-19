# WxElevator 문 위치 C++ 제어 제거 (StateTree ComponentMove 단독 구동)

## 계획

### 목표
`AWxElevator`에서 문 위치 제어(`SetDoorOpenAlpha` 및 관련 멤버)를 들어내, StateTree `ComponentMove`가 문을 단독으로 구동하게 한다. 현재 Closed→Arrived 직행 시 `SnapVisualsToState`가 `SetDoorOpenAlpha(1.f)`로 문을 즉시 열어 ComponentMove의 부드러운 슬라이드를 덮어쓰는 버그를 제거한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxElevator.cpp` | SnapVisualsToState 문 알파 제거, BeginPlay 문 캐시/오프셋 제거, SetDoorOpenAlpha·ComputeDoorWidth 정의 제거, Engine/StaticMesh.h include 제거 | 수정 |
| `WxElevator.h` | 문 게터/세터(SetDoorOpenAlpha·GetDoorOpenAlpha·GetDoorAnimDuration·ComputeDoorWidth)·멤버(DoorAnimDuration·CurrentOpenAlpha·Closed/OpenOffset 위치 캐시)·관련 doc 제거 | 수정 |

### 접근 방식
- **문 위치를 C++에서 격하**: ComponentSplineMove 워크로그가 후속 과제로 남긴 "고아 프리미티브 정리"의 문 부분. 문은 복제와 무관(`CurrentOpenAlpha` 비복제 로컬)해 플랫폼과 분리해 안전하게 제거 가능. 플랫폼은 복제·복원 스냅 주체 문제로 별도.
- **DoorLeft/DoorRight 컴포넌트 유지**: StateTree ComponentMove가 바인딩하는 대상이므로 보존.
- **의존성**: 제거 후 문 개폐는 전적으로 StateTree 재선택(Closed↔Arrived)에 의존. 재선택이 끊겨 있으면 문이 안 열리며, 이는 별도(자산) 수정 대상이자 본 변경이 겸하는 진단.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxElevator.cpp` | SnapVisualsToState에서 문 알파(NewAlpha·SetDoorOpenAlpha) 제거(Closed/Arrived 거리 case 통합), BeginPlay 문 캐시/오프셋 블록 제거, SetDoorOpenAlpha·ComputeDoorWidth 정의 제거, `Engine/StaticMesh.h` include 제거 | 수정 |
| `WxElevator.h` | 문 게터/세터(SetDoorOpenAlpha·GetDoorOpenAlpha·GetDoorAnimDuration·ComputeDoorWidth)·멤버(DoorAnimDuration·CurrentOpenAlpha·Door위치 캐시 4개) 제거, 위치 정보·SnapVisualsToState doc 정정, GetElevatorState doc의 삭제된 태스크 언급 정리 | 수정 |

### 구현·결정과 그 이유
- **문 위치를 C++에서 완전 격하**: Closed→Arrived 직행 시 `SetDoorOpenAlpha(1.f)`가 그 프레임에 문을 즉시 열어 StateTree ComponentMove의 부드러운 슬라이드를 덮어쓰던 게 원인. C++ 문 제어를 들어내 ComponentMove가 단독으로 문을 구동하게 했다. 라이브 전이는 슬라이드, 초기 진입(복원/시작)은 ComponentMove가 목표로 스냅하므로 C++ 스냅이 불필요.
- **문/플랫폼 분리**: 문은 복제와 무관(`CurrentOpenAlpha` 비복제 로컬)해 안전하게 제거. 플랫폼은 복제·복원 스냅 주체 문제가 얽혀 SnapVisualsToState의 `SetPlatformDistance`를 그대로 유지하고 범위에서 제외.

### 계획 대비 달라진 점
- 계획대로. (Closed/Arrived가 동일 플랫폼 거리라 두 case를 fall-through로 합쳤고, GetElevatorState doc이 삭제된 ElevatorMove/ElevatorDoorPose를 언급해 인접 정리로 함께 갱신.)

### 후속 과제
- **재선택 의존(자산)**: 이제 문 개폐가 StateTree 재선택(Closed↔Arrived)에 전적으로 의존. 변경 후 문이 안 열리면 Closed/Arrived의 `On Tick → Root` 전이가 끊긴 것 → 자산 수정 필요. (본 변경이 그 진단을 겸함.)
- **클래스 doc 잔여 드리프트**: 클래스 헤더 상단의 상태머신 서술(Moving 부모/자식 순차, ExpectedState→Succeeded 재선택 등)이 삭제된 옛 노드 메커니즘을 여전히 기술. 재선택·승급 설계가 확정되면 일괄 정정 필요.
- **플랫폼 프리미티브 정리**: GetMoveDuration/GetTargetDistance/GetPlatformDistance/GetSplineLength 등은 옛 ElevatorMove만 쓰던 고아 가능성. 복제 영향 검토와 함께 별도 작업.
- **PIE 검증 미완**: 컴파일만 확인. 같은 끝점 콘솔 상호작용 시 문 부드러운 개방, 복원/시작 시 초기 문 위치 정합 확인 필요.
