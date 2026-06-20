# WxGimmick StateTree 자동 시작 전환

## 계획

### 목표
`AWxGimmick` 베이스의 StateTree 를 자동 시작(`SetStartLogicAutomatically(true)`)으로 되돌리고, 자식 5종의 명시 `StartLogic()` 호출을 제거한다. 자식이 StartLogic 시점을 통제하던 유일한 이유였던 초기 위치/포즈 스냅이 이제 전부 태스크 `EnterState` 안에서 자체 수행되므로 더는 필요 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxWorld/.../Gimmick/WxGimmick.cpp` | `SetStartLogicAutomatically(false)`→`(true)`, 주석 교체(자동 시작 의존·태스크 자체 스냅) | 수정 |
| `WxWorld/.../Gimmick/WxDoor.cpp` | `BeginPlay` 말미 `StateTree->StartLogic();` + 부속 주석 제거 | 수정 |
| `WxWorld/.../Gimmick/WxElevator.cpp` | 동일 제거 | 수정 |
| `WxWorld/.../Gimmick/WxTreasureChest.cpp` | 동일 제거 | 수정 |
| `WxWorld/.../Gimmick/WxAlarmConsole.cpp` | 동일 제거 | 수정 |
| `WxWorld/.../Gimmick/WxSpawnConsole.cpp` | 동일 제거 | 수정 |

StateTree 미사용 3종(`WxCutsceneTrigger`·`WxCheckPoint`·`WxLaserCorridor`)은 손대지 않는다. 자동 시작 시 애셋이 없어 `ValidateStateTreeReference`가 인스턴스당 Error 로그를 남기지만 `StartTree`는 조용히 early-return해 동작 영향이 없고, 사용자가 로그를 허용했다.

### 접근 방식
- **자동 시작 + 명시 호출 제거**: 자동 시작이면 `StartLogic`이 `UStateTreeComponent::BeginPlay()`(자식의 `Super::BeginPlay()` 내부 디스패치)에서 실행된다. 자식이 그 뒤 하는 `OnInteracted` 바인딩은 반응형 콜백이라 늦게 묶여도 시작 정합에 무해하고, 초기 진입 스냅은 태스크 `EnterState`(`!SourceStateID.IsValid()`)가 전담하므로 호출 시점과 무관하게 정확하다.
- **이중 호출 차단**: `StartTree`에 `bIsRunning` 가드가 없어 자동+명시 이중 호출은 재시작을 부르므로, 자식의 명시 호출을 전부 제거해 자동 시작 1회만 남긴다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxWorld/.../Gimmick/WxGimmick.cpp` | `SetStartLogicAutomatically(true)`, 주석을 자동 시작 의존·태스크 자체 스냅 취지로 교체 | 수정 |
| `WxWorld/.../Gimmick/WxDoor.cpp` | `StateTree->StartLogic()`+부속 주석 제거, 죽은 `StateTreeComponent.h` include 제거 | 수정 |
| `WxWorld/.../Gimmick/WxElevator.cpp` | 동일 | 수정 |
| `WxWorld/.../Gimmick/WxTreasureChest.cpp` | 동일 | 수정 |
| `WxWorld/.../Gimmick/WxAlarmConsole.cpp` | 동일 | 수정 |
| `WxWorld/.../Gimmick/WxSpawnConsole.cpp` | 동일 | 수정 |

### 구현·결정과 그 이유
- **자동 시작이 안전한 이유 재확인(UE 5.7 소스)**: 자동 시작 시 `StartLogic`은 `UStateTreeComponent::BeginPlay()`에서, 즉 자식의 `Super::BeginPlay()` 내부 컴포넌트 디스패치 도중 실행된다. 자식이 그 뒤 수행하는 일은 반응형 `OnInteracted` 바인딩뿐이라 시작 정합과 무관하고, 위치·포즈·애니 초기 스냅은 태스크 `EnterState`의 초기 진입 분기가 전담하므로 호출 시점에 의존하지 않는다.
- **이중 호출 제거**: `UStateTreeComponent::StartTree`에 `bIsRunning` 가드가 없어 자동+명시가 겹치면 재시작이 일어난다. 그래서 자식의 명시 호출을 전부 없애 자동 1회만 남겼다.
- **미사용 3종은 방치**: `WxCutsceneTrigger`·`WxCheckPoint`·`WxLaserCorridor`는 StateTree 애셋이 없어 자동 시작 시 `ValidateStateTreeReference`가 인스턴스당 Error 로그를 남기지만 `StartTree`가 조용히 early-return해 동작 영향이 없다. 사용자가 로그를 허용해 별도 opt-out을 두지 않았다.

### 계획 대비 달라진 점
- 자식 5종에서 더는 쓰이지 않게 된 `#include "Components/StateTreeComponent.h"`를 함께 제거했다(계획엔 명시 안 했으나 StartLogic 제거의 직접 귀결이라 정리). 본문에 남은 `StateTree`는 전부 주석뿐이라 컴파일 무영향, 빌드로 확인됨.

### 후속 과제
- 없음. WxEditor(Development) 빌드 성공(6개 cpp 컴파일·링크 통과).

