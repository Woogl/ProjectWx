# 스플라인 경로 이동 StateTree 태스크 (ComponentSplineMove)

## 계획

### 목표
컴포넌트를 스플라인 경로를 따라 옮기는 범용 mover `FWxStateTreeTask_ComponentSplineMove` 를 기믹 공용 노드에 추가한다. 진입 시 컴포넌트 현재 위치에서 가장 가까운 스플라인 포인트를 찾아, 그 다음 포인트까지 스플라인 곡선을 따라 일정 속도로 이동시킨다. 기존 직선 mover `FWxStateTreeTask_ComponentMove` 와 같은 철학(State 무관 순수 비주얼, 도달 후 hold, 전이는 에셋 조건이 구동)을 따른다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_ComponentSplineMove` + InstanceData 선언(ComponentMove 뒤), `class USplineComponent;` 전방 선언, 상단 주석 노드 한 줄 추가 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | EnterState/Tick/GetDescription 구현, `#include "Components/SplineComponent.h"` | 수정 |

`USplineComponent` 는 Engine 모듈이라 Build.cs 변경 불필요(`WxElevator.h` 가 이미 동일 헤더 사용).

### 접근 방식
- **곡선 추종(distance-along-spline)**: 엘리베이터(`AWxElevator`)가 액터에 묶어 쓰는 곡선 추종을 State·기믹 무관한 범용 태스크로 일반화. 좌표계는 World(부모 관계 무가정), 컴포넌트는 스플라인 위를 탄다고 가정.
- **한 번의 진입 = 한 세그먼트**: EnterState 가 nearest 포인트를 찾고 다음 포인트까지 거리 구간을 잡으면 Tick 이 일정 속도로 보간. 재진입 시 nearest 가 직전 목표로 잡혀 한 칸 더 전진(포인트를 차례로 밟음). 전이는 에셋 Enum Compare 가 구동.
- **InstanceData**: 저자 편집 `TargetComponent`/`Spline`/`Duration`(세그먼트 주파 시간). 런타임 스크래치 `CurrentDistance`/`TargetDistance`/`MoveSpeed`(nearest 가 동적이라 진입 시 캡처; ComponentMove 와 달리 고정 저자값에서 재계산 불가).
- **끝점**: nearest 가 마지막일 때 닫힌 루프면 목표=`GetSplineLength()` 로 폐합 구간 지나 0번 위치 wrap, 열린 스플라인이면 목표=시작이라 이동 없이 hold.
- **스냅 vs 슬라이드**(ComponentMove 동일): 초기 진입(`SourceStateID` 무효)·`Duration<=0`·이미 목표면 즉시 목표 스냅, 라이브 전이면 Tick 슬라이드.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_ComponentSplineMove` + InstanceData 선언, `USplineComponent` 전방 선언, 상단 주석 노드 한 줄 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | EnterState/Tick/GetDescription 구현, `Components/SplineComponent.h` include | 수정 |
| `WxElevatorStateTreeNodes.h/.cpp` | 삭제. ST_Elevator 가 ComponentSplineMove(플랫폼)+ComponentMove(문)로 재오서링되어 `ElevatorMove`/`ElevatorDoorPose` 가 어떤 에셋에서도 미참조(죽은 코드) | 삭제 |

### 구현·결정과 그 이유
- **곡선 추종 + World 공간**: 엘리베이터가 액터에 묶어 쓰던 distance-along-spline 추종을 범용 태스크로 일반화. 좌표계는 World 로 잡아 스플라인과 컴포넌트의 부모 관계를 가정하지 않게 했다(컴포넌트가 스플라인 경로 위를 탄다는 점만 가정).
- **세그먼트당 진입 + 런타임 캡처**: nearest 포인트는 진입 시점에만 정해지는 동적 값이라 ComponentMove 처럼 고정 저자값에서 재계산할 수 없다. 그래서 시작·목표·속도를 EnterState 에서 1회 산출해 인스턴스 런타임 필드(`CurrentDistance`/`TargetDistance`/`MoveSpeed`)에 캡처하고, Tick 은 그 고정 속도로만 보간한다. 재진입 시 다시 nearest 를 잡으므로 포인트를 차례로 밟는다.
- **속도 일정성**: 속도를 세그먼트 호 길이/Duration 로 한 번 고정해, 보간으로 줄어든 잔여 거리로 재계산하지 않는다(감속 없음). ComponentMove 의 LocalOffset 고정 속도와 같은 원리.
- **끝점**: 닫힌 루프는 마지막 포인트의 다음을 `GetSplineLength()`(폐합 구간 끝 = 0번 포인트 위치)로 잡아 곡선을 따라 wrap, 열린 스플라인은 목표=시작이라 이동 없이 hold.

### 계획 대비 달라진 점
- 계획대로. (포인트 2개 미만이면 이동 세그먼트가 없어 Running 으로 hold 하는 가드만 명시적으로 넣음)

### 후속 과제
- ST_Door/ST_Elevator 등 에셋에 이 태스크를 배선하고 전이 조건을 author 하는 것은 수동(사용자) 단계. (ST_Elevator 는 이미 ComponentSplineMove+ComponentMove 로 재오서링 확인됨)
- 세그먼트 중간 강제 재진입 시 nearest 포인트로 스냅 후 진행(포인트 단위 양자화) — 요청 의도대로의 특성이며, 정지 시 항상 포인트에 머무는 사용에선 드러나지 않음.
- **WxElevator 잔여 정리(미수행, 사용자 판단)**: 삭제된 ElevatorMove/ElevatorDoorPose 만 소비하던 프리미티브(SetPlatformDistance·GetTargetDistance·GetMoveDuration·GetDoorOpenAlpha·SetDoorOpenAlpha·GetDoorAnimDuration 등)와 이들을 언급하는 주석이 WxElevator.h/.cpp 에 고아로 남음. 컴파일은 정상이나 죽은 코드일 가능성 높음. 복제 영향이 있어 별도 작업으로 분리.
