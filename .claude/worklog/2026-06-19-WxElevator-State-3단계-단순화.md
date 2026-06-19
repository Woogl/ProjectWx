# WxElevator State 3단계 단순화 (Closed / Moving / Arrived)

## 계획

### 목표
`AWxElevator`의 권위 State 5개(DoorsClosed/DoorsOpening/DoorsOpen/DoorsClosing/Moving)를 3개(`Closed`/`Moving`/`Arrived`)로 줄인다. 문 개폐는 본질적으로 비주얼 보간이므로 권위 State에서 빼고 StateTree Task로만 처리해, 권위 State를 정지 2개 + 이동 1개로 단순화한다. Moving은 내부에서 "문 닫기 → 플랫폼 이동"을 순차 수행하고, Arrived 진입 시 문을 연다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxElevator.h` | enum 5→3, State 디폴트 Closed, 클래스/필드 doc 전면 갱신 | 수정 |
| `WxElevator.cpp` | BeginMoveSequence 단순화, SnapVisualsToState 3-state 재작성(Moving 알파 유지), 가드 enum 갱신 | 수정 |
| `WxElevatorStateTreeNodes.h` | ElevatorDoorPose에 bCompleteOnReached+ExpectedState, ElevatorMove ExpectedState, EnteredState 제거, doc 갱신 | 수정 |
| `WxElevatorStateTreeNodes.cpp` | 위 Task 로직을 ExpectedState watch 기준으로 재작성 | 수정 |

### 접근 방식
- **문 애니를 권위 State에서 격하**: DoorsOpening/DoorsClosing 전이 단계를 제거하고, 문 개폐는 StateTree Task의 비주얼 보간으로만 처리한다. Door의 "State=확정 목표, 슬라이드는 순수 비주얼" 철학과 정렬.
- **watch 기준을 `ExpectedState`로 전환**: 기존 Task는 진입 시점 권위 State를 캡처(`EnteredState`)해 watch했으나, 3-state에선 한 권위 State(Moving)를 두 자식(CloseDoors/MovePlatform)이 공유하므로 진입 캡처가 데드락을 부른다(클라가 CloseDoors에 머무는 동안 서버가 Arrived를 먼저 복제하면, 뒤늦게 진입한 MovePlatform이 Arrived를 캡처해 watch가 안 걸리고 고착). 노드마다 고정 `ExpectedState` 파라미터를 두고 `State != ExpectedState → Succeeded`로 통일하면 enter condition과 대칭이 되어 어느 단계에서 진입하든 올바른 상태로 수렴한다.
- **Moving 순차는 자식 상태로**: StateTree 한 상태의 여러 Task는 동시 실행이므로, "문 닫기 → 이동" 순차는 Moving 부모의 두 자식(CloseDoors→MovePlatform)으로 표현한다. CloseDoors의 DoorPose는 complete 모드(닫힘 도달 시 Succeeded)라 이미 닫혀 있으면 즉시 통과한다.
- **에셋 author는 사용자(에디터)**: ST_Elevator 재author는 빌드 후 수동.

### 전이 흐름
| 현재 | 호출 | 결과 |
|---|---|---|
| Closed | 같은 끝점 | → Arrived (문만 열기) |
| Closed | 다른 끝점 | → Moving (즉시 이동) → Arrived |
| Arrived | 같은 끝점 | 노옵 |
| Arrived | 다른 끝점 | → Moving (문 닫기→이동) → Arrived |

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxElevator.h` | enum 5→3(Closed/Moving/Arrived), State 디폴트 Closed, 클래스/필드 doc 전면 갱신 | 수정 |
| `WxElevator.cpp` | BeginMoveSequence 단순화(다른 끝점이면 항상 Moving), SnapVisualsToState 3-state 재작성(Moving 알파 유지), 인터랙션/이동 가드 enum 갱신 | 수정 |
| `WxElevatorStateTreeNodes.h` | ElevatorMove/ElevatorDoorPose InstanceData에 ExpectedState 도입(EnteredState 제거), DoorPose에 bCompleteOnReached 추가·PromoteToState 제거, doc 갱신 | 수정 |
| `WxElevatorStateTreeNodes.cpp` | 두 Task의 watch를 ExpectedState 기준으로 재작성, DoorPose hold/complete 분기 구현, GetDescription 갱신 | 수정 |

### 구현·결정과 그 이유
- **문 애니를 권위 State에서 격하**: 여닫는 중을 별도 권위 상태로 두던 두 단계를 없애고 문 개폐를 StateTree Task의 비주얼 보간으로만 처리했다. 권위 State가 정지 2 + 이동 1로 줄어 복제·세이브·전이 흐름이 단순해지고, Door의 "State=확정 목표, 슬라이드는 순수 비주얼" 철학과 정렬된다.
- **watch 기준을 진입 캡처에서 ExpectedState로 전환(데드락 방지)**: 한 권위 State(Moving)를 두 자식(문 닫기·이동)이 공유하게 되면서, 진입 시점 State를 캡처하는 기존 방식은 클라가 문 닫기 단계에 머무는 동안 서버가 도착을 먼저 복제하면 이동 노드가 최종값을 캡처해 고착되는 데드락을 만든다. 각 노드가 자기 소속 상태를 고정 파라미터로 들고 "현재 State가 그 값을 벗어나면 완료"하도록 바꿔, 엔진 EnumCompare 진입 조건과 대칭을 이루고 어느 단계에서 진입하든 올바른 상태로 수렴하게 했다.
- **순차는 자식 상태, 문 닫기는 complete 모드**: StateTree 한 상태의 Task는 동시 실행이라 "문 닫기 → 이동" 순차는 Moving 부모의 두 자식으로 표현한다. 문 닫기 Task는 도달 시 완료(complete)라 이미 닫혀 있으면 즉시 통과하고, 정지 상태의 문 Task는 도달 후 hold하여 두 쓰임을 한 노드가 파라미터로 겸한다.
- **Moving 진입 시 문 알파 유지**: 위치 스냅은 출발 끝점으로 보정하되 문 알파는 건드리지 않아, 열린 상태에서 출발하면 문 닫기 애니가 1→0으로 보이고 복원/레이트조인은 디폴트 0이라 즉시 통과한다.

### 계획 대비 달라진 점
- 계획대로. (설계 단계에서 직접 검증으로 데드락을 발견해 ExpectedState 전환을 plan에 미리 반영했고, 그대로 구현했다.)

### 후속 과제
- **ST_Elevator 에셋 재author(사용자, 에디터)**: 3개 상태(Closed/Arrived 최상위 + Moving 부모와 두 자식 CloseDoors→MovePlatform). 각 상태 enter condition은 엔진 Enum Compare(State==해당값), 각 Task에 ExpectedState·bOpen·bCompleteOnReached·PromoteToState를 plan 표대로 설정. 인터랙션은 정지 true/이동 false. author 후 BP_Elevator의 StateTree 컴포넌트를 ST_Elevator로 재할당(현재 ST_Door 참조 중).
- **PIE 검증 미완**: 컴파일만 확인(WxEditor Development, Succeeded). 에셋 재author 후 호출/이동/문 시퀀스, 2클라 서버권위 게이팅, 이동 중 SaveGame→Load 복원 정합 확인 필요.
