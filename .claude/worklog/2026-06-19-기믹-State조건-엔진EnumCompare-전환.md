# 기믹 State enter condition 을 엔진 Enum Compare 로 전환 (Door·Elevator)

## 계획

### 목표
ST_Door / ST_Elevator 의 enter condition 으로 쓰는 커스텀 조건 2종(`FWxStateTreeCondition_DoorStateIs`, `FWxStateTreeCondition_ElevatorStateIs`)을 제거하고 엔진 기본 **Enum Compare**(`FStateTreeCompareEnumCondition`)로 대체한다. 이를 위해 각 액터의 권위 `State` 를 StateTree 바인딩 소스로 노출한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxWorld/.../Gimmick/WxDoor.h` | `State` 에 `VisibleAnywhere, Category="Wx", meta=(AllowPrivateAccess="true")` 추가(`Replicated, SaveGame` 유지). DoorStateIs 언급 doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxElevator.h` | `State` 에 동일 추가(`ReplicatedUsing=OnRep_State, SaveGame` 유지). ElevatorStateIs 언급 doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxDoorStateTreeNodes.h/.cpp` | `FWxStateTreeCondition_DoorStateIs`(+InstanceData/TestCondition/GetDescription) 삭제, 헤더 doc 정리, 불필요해진 `StateTreeConditionBase.h` include 제거 | 수정 |
| `WxWorld/.../Gimmick/WxElevatorStateTreeNodes.h/.cpp` | `FWxStateTreeCondition_ElevatorStateIs` 동일 삭제·정리 | 수정 |
| `WxWorld/README.md` | 노드 목록에서 StateIs 제거, enter condition = 엔진 Enum Compare(State 바인딩)로 갱신 | 수정 |

### 접근 방식
- **엔진 Enum Compare = `Left`(바인딩) vs `Right`(리터럴 enum) 비교**: 각 비주얼 상태의 enter condition 을 커스텀 `StateIs` 대신 엔진 조건으로 둔다. `Left` 를 컨텍스트 액터의 `State` 에 바인딩하고 `Right` 를 그 상태 리터럴로 둔다. 커스텀 조건이 하던 "현재 State == 이 상태" 검사를 엔진 노드가 그대로 수행한다.
- **`State` 노출은 지정자 2개로 최소화**: 엔진의 바인딩 가능 판정(`IsPropertyBindable`)은 ① `CPF_Edit`(=`VisibleAnywhere`) ② private 면 `meta=(AllowPrivateAccess="true")` 둘만 요구한다. enum 을 `BlueprintType` 으로 바꾸거나 `BlueprintReadOnly` 를 붙일 필요가 없다 — `State` 에 `VisibleAnywhere`+`AllowPrivateAccess` 만 추가한다(복제/세이브 지정자는 유지).
- **게터·태스크는 유지**: `GetDoorState()`/`GetElevatorState()` 는 전이 태스크의 `EnteredState` watch 가 계속 쓰므로 남긴다(StateTree 바인딩은 필드를, C++ 태스크는 게터를 읽어 소비자가 다르다). 태스크 3~4종·`OnRep_*`·`ApplyState` 불변.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxWorld/.../Gimmick/WxDoor.h` | `State` 에 `VisibleAnywhere, Category="Wx", meta=(AllowPrivateAccess="true")` 추가(`Replicated, SaveGame` 유지), 게터·클래스 doc 의 DoorStateIs 언급 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxElevator.h` | `State` 에 동일 추가(`ReplicatedUsing, SaveGame` 유지), 게터 doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxDoorStateTreeNodes.h/.cpp` | `FWxStateTreeCondition_DoorStateIs` 전부 삭제, `StateTreeConditionBase.h` include 제거, 노드 목록 doc(셋→둘) 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxElevatorStateTreeNodes.h/.cpp` | `FWxStateTreeCondition_ElevatorStateIs` 전부 삭제, include·doc(넷→셋) 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxDoor.cpp` `WxElevator.cpp` | StartLogic 직전 초기 선택 주석의 StateIs 언급 갱신 | 수정 |

### 구현·결정과 그 이유
- **`State` 노출은 지정자 2개로 최소화**: 엔진의 바인딩 가능 판정(`IsPropertyBindable`)이 ① `CPF_Edit`(=`VisibleAnywhere`) ② private 면 `AllowPrivateAccess` 둘만 요구함을 엔진 소스로 확인했다. enum 을 `BlueprintType` 으로 바꾸거나 `BlueprintReadOnly` 를 붙일 필요가 없어, 복제/세이브 지정자를 유지한 채 두 지정자만 추가했다. 최소 노출로 모든 BP 그래프에 변수가 새는 것을 피했다(Details 에 읽기전용으로만 표시).
- **게터·태스크 유지(소비자 분리)**: `GetXxxState()` 게터는 전이 태스크의 `EnteredState` watch 가 계속 쓰므로 남겼다. 결과적으로 StateTree 바인딩은 `State` 필드를, C++ 태스크는 게터를 읽는다 — 약간의 중복이나 소비 경로가 달라 불가피하고 무해하다.
- **조건만 제거, 전이 메커니즘 불변**: 삭제한 건 enter condition 용 커스텀 조건뿐이다. "권위 State 변경 → 태스크가 Succeeded → On Succeeded → Root 재선택" 전이 메커니즘과 태스크/`OnRep`/`ApplyState` 는 그대로다.

### 계획 대비 달라진 점
- **README 변경 없음**: 계획엔 README 수정을 넣었으나, README 가 StateIs 를 직접 거명하지 않고 "C++는 노드용 프리미티브만 노출" 서술이 조건 제거 후 오히려 더 정확해져 손대지 않았다.

### 후속 과제
- **에디터 에셋 재작성(사용자, 필수)**:
  - `ST_Door`: 커스텀 조건 삭제로 기존 `Wx Door State Is` 노드가 무효화된다. 각 상태(Close/Open) enter condition 을 엔진 `Enum Compare` 로 교체 — `Left → (Door 컨텍스트)State` 바인딩, `Right = Close/Open` 리터럴.
  - `ST_Elevator`(신규 author): 5개 상태 enter condition 도 동일 패턴(`Enum Compare`, `Left→State` 바인딩, `Right=각 상태`).
- **검증 미완**: 컴파일만 확인(WxEditor Development, Succeeded). 에셋 재작성 후 PIE 에서 Door 개폐·Elevator 호출/이동/문 시퀀스 및 2클라 서버권위 게이팅 확인 필요.
