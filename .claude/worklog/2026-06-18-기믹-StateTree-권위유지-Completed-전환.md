# 기믹 StateTree 전이를 권위 유지 Completed 방식으로 전환 (Door·Elevator)

## 계획

### 목표
Door·Elevator StateTree 전이를 `Event.Gimmick.StateChanged` 태그·`SendStateTreeEvent` 없이 StateTree 네이티브 Completed(`On Completed → Root` 재선택)로 바꾸되, **서버 권위 전이 게이팅을 100% 유지**한다. 태그는 코드에서 완전 제거한다.

### 핵심 메커니즘 (권위 유지 Completed)
전이 태스크가 EnterState에서 선택 시점의 권위 `State`를 `EnteredState`로 기록하고, **`GetState() != EnteredState` 일 때만 `Succeeded`를 반환**한다.
- 서버: 로컬 애니/이동 완료 시 권위에서 `SetXxxState(Next)` 승급 → 같은 틱에 `State != EnteredState` → `Succeeded` → `On Completed → Root` 재선택.
- 클라: 로컬 완료해도 `SetXxxState`는 노옵이라 `State` 불변 → 태스크 계속 `Running`. 복제로 `State`가 바뀐 뒤에야 `Succeeded`. = 서버가 클라 전이를 게이팅(기존 이벤트 방식과 동일).
- `EnteredState` 기록 덕에 재선택 직후 즉시 재완료 루프 없음(새 상태는 `State == EnteredState`).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCore/.../WxGameplayTags.h/.cpp` | `Event_Gimmick_StateChanged` 선언·정의 제거 | 수정 |
| `WxWorld/.../Gimmick/WxDoor.h/.cpp` | `ApplyState` override 제거(불필요), `OnRep_State` 제거하고 `State`를 plain `Replicated`로, `SetDoorState`에서 `ApplyState` 호출 제거, `WxGameplayTags` include 제거, doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxDoorStateTreeNodes.h/.cpp` | `DoorPose` 인스턴스에 런타임 `EnteredState`; EnterState 기록, Tick/EnterState 끝에서 State-watch → `Succeeded`/`Running` | 수정 |
| `WxWorld/.../Gimmick/WxElevator.h/.cpp` | `ApplyState`를 `SnapVisualsToState`만으로(SendEvent 제거), `WxGameplayTags` include 제거. `OnRep_State`/`OnRep_TargetEndpoint`→`ApplyState`(=SnapVisualsToState) 유지(위치 스냅 필요), doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxElevatorStateTreeNodes.h/.cpp` | `ElevatorMove`·`ElevatorDoorPose` 인스턴스에 런타임 `EnteredState`; EnterState 기록, 승급 후 State-watch → `Succeeded`/`Running` | 수정 |

### 접근 방식
- **Door**: 전이가 상호작용 구동뿐이라 `State` 변경 감지=상호작용 반응. `ApplyState`/`OnRep_State` 불필요(DoorPose가 매 틱 `State` 폴링). `State`는 plain `Replicated, SaveGame`.
- **Elevator**: `State` 변경 시 위치/문 알파 스냅이 필요해 `ApplyState`(=SnapVisualsToState)와 `OnRep_State`/`OnRep_TargetEndpoint`를 유지하되 SendEvent만 제거. 전이는 태스크 State-watch가 구동.
- **에셋(사용자)**: 두 ST의 Root `On Event StateChanged → Root` 전이를 각 상태의 `On State Succeeded → Root`로 교체. 태스크는 State 전진 감지 시에만 `Succeeded`를 반환하므로 `Succeeded` 트리거가 의도와 정확히 일치하고, 방어용 `Failed`(null 오너)는 정상 전이로 흘리지 않는다. 태스크/조건은 그대로.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCore/.../WxGameplayTags.h/.cpp` | `Event_Gimmick_StateChanged` 선언·정의 제거 | 수정 |
| `WxWorld/.../Gimmick/WxDoor.h/.cpp` | `ApplyState`·`OnRep_State` 제거, `State`를 plain `Replicated, SaveGame`로, `SetDoorState`에서 `ApplyState` 호출 제거, `WxGameplayTags` include 제거, doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxDoorStateTreeNodes.h/.cpp` | `DoorPose` 인스턴스에 런타임 `EnteredState`; EnterState 기록, Tick 첫머리에 `State != EnteredState → Succeeded` watch | 수정 |
| `WxWorld/.../Gimmick/WxElevator.h/.cpp` | `ApplyState`를 `SnapVisualsToState`만으로(SendEvent·`WxGameplayTags` 제거), `OnRep_State`/`OnRep_TargetEndpoint`→`ApplyState`(=스냅) 유지, doc 갱신 | 수정 |
| `WxWorld/.../Gimmick/WxElevatorStateTreeNodes.h/.cpp` | `ElevatorMove`·`ElevatorDoorPose` 인스턴스에 런타임 `EnteredState`; EnterState 기록, 승급 후 끝에서 watch | 수정 |
| `WxWorld/README.md` | 전이 메커니즘 서술을 State-watch+`On Succeeded→Root`로 갱신 | 수정 |

### 구현·결정과 그 이유
- **권위 유지 Completed = "권위 State가 EnteredState를 떠나면 Succeeded"**: 태스크가 EnterState에서 선택 시점 State를 기록하고, `GetXxxState() != EnteredState`일 때만 `Succeeded`를 반환한다. 서버는 로컬 완료 시 `SetXxxState` 승급으로 같은 틱에 완료하고, 클라는 `SetXxxState`가 노옵이라 복제로 State가 바뀐 뒤에야 완료한다. 결과적으로 "클라 전이를 서버가 게이팅"하는 기존 이벤트 방식의 권위 보장이 그대로 유지되면서 이벤트 태그·`SendStateTreeEvent`가 사라진다.
- **트리거는 `On Succeeded`(=Completed 아님)**: 태스크는 State 전진을 감지했을 때만 `Succeeded`를 반환하므로 `Succeeded` 트리거가 의도와 정확히 일치한다. 방어용 `Failed`(null 오너)는 정상 전이로 흘리지 않는다.
- **watch 위치는 책임에 따라**: Door의 DoorPose는 자기 승급이 없어(상호작용만 State 변경) Tick 첫머리에서 watch. Elevator의 Move/DoorPose는 자기 승급이 있어 보간·승급 뒤(끝)에서 watch해 서버의 같은-틱 승급을 잡는다. 둘 다 `EnteredState` 기록으로 재선택 직후 즉시 재완료 루프를 막는다.
- **Door는 `ApplyState`/`OnRep_State` 제거, Elevator는 유지**: Door는 State 변경 시 할 일이 없어(DoorPose가 매 틱 폴링) `State`를 plain `Replicated`로 두고 둘 다 제거. Elevator는 State 변경 시 위치/문 알파 스냅이 필요해 `ApplyState`(=SnapVisualsToState)와 OnRep들을 유지하되 SendEvent만 뺐다.

### 계획 대비 달라진 점
- 트리거를 `On State Completed` → `On State Succeeded`로 변경(구현 중 사용자 질문 반영). C++는 `Succeeded` 반환이라 동일, 에셋 트리거만 더 정확한 선택.

### 후속 과제
- **ST_Door·ST_Elevator 에셋 전이 재작성(사용자, 필수)**: 두 ST의 기존 `Root: On Event Event.Gimmick.StateChanged → Root` 전이를 각 상태(또는 공통 부모)의 **`On State Succeeded → Root`** 재선택으로 교체. 태스크/조건/StateIs는 그대로. (Moving 상태는 `ElevatorMove`+`Interaction(disabled)`만으로 충분 — 문은 SnapVisualsToState가 0으로 스냅해 유지하므로 DoorPose 불요.) 이 지침이 앞선 두 워크로그의 "On Event StateChanged → Root" 지침을 대체한다.
- **검증 미완**: 컴파일만 확인. 에셋 재작성 후 PIE에서 Door 개폐·Elevator 호출/이동/문 시퀀스 및 2클라 서버권위 게이팅(클라가 서버 State 전에 앞서가지 않는지) 확인 필요.
