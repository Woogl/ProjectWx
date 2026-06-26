# FWxStateTreeTask_SetState 제거 (ST 의 State 직접 쓰기 태스크 폐기)

## 계획

### 목표
ST 태스크가 권위 측에서 기믹 State enum 을 직접 확정할 수 있게 하던 공용 노드 `FWxStateTreeTask_SetState` 를 제거한다. ST 가 상호작용 트리거 없이 스스로 State 전이를 구동하는 경로가 위험하다고 판단해 폐기한다. 어떤 ST 에셋도 이 노드를 배치한 적이 없어(채택 0) 깨질 에셋이 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | 상단 노드 목록 주석의 `SetState` 1줄 + `// ── SetState ──` 섹션(InstanceData 구조체 + 태스크 구조체) 삭제 | 삭제 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `// ── SetState ──` 섹션(생성자·EnterState·GetDescription) 삭제 + 유일 사용처가 사라지는 `#include "Gimmick/WxGimmick.h"` 삭제 | 삭제 |
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmick.h` | 클래스 doc 주석 2곳의 "·Wx Set State 태스크" 문구 삭제(이제 CommitGimmickState 호출자는 인터랙션 핸들러뿐) | 주석 |

### 접근 방식
- **태스크만 제거, 베이스 인프라 존치**: `CommitGimmickState`/`SetGimmickState` 는 Door·Elevator·TreasureChest·SpawnConsole·AlarmConsole·LaserCorridor 인터랙션 핸들러가 공유하는 핵심 권위 쓰기 경로라 그대로 둔다. 사라지는 것은 ST→State 직접 쓰기 경로 하나뿐이다.
- **include 정리**: cpp 에서 `AWxGimmick` 캐스트는 SetState 의 EnterState 가 유일 사용처라, 제거 후 `WxGimmick.h` include 도 함께 뺀다(데드 include 지양). `WxSpawner.h` 등은 TriggerSpawners 가 쓰므로 유지.
- **주석 정합성**: 제거된 태스크를 가리키는 `WxGimmick.h` 의 살아있는 주석을 갱신해 "호출자=인터랙션 핸들러"로 정리한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | 상단 노드 목록 주석의 SetState 1줄 + `// ── SetState ──` 섹션(InstanceData + 태스크 구조체) 삭제 | 삭제 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `// ── SetState ──` 섹션(생성자·EnterState·GetDescription) + 유일 사용처가 사라진 `#include "Gimmick/WxGimmick.h"` 삭제 | 삭제 |
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmick.h` | 클래스 doc 주석 2곳의 "·Wx Set State 태스크" 문구 삭제 | 주석 |

### 구현·결정과 그 이유
- **베이스 인프라 존치**: `CommitGimmickState`/`SetGimmickState` 는 Door·Elevator·TreasureChest·SpawnConsole·AlarmConsole·LaserCorridor 인터랙션 핸들러 6종이 공유하는 핵심 권위 쓰기 경로라 그대로 뒀다. 사라진 건 ST→State 직접 쓰기 경로 하나뿐이다.
- **에셋 영향 0 (바이너리 검증)**: 처음의 ripgrep `Content/**/*.uasset` 무매칭은 ripgrep이 바이너리를 기본 스킵하므로 신뢰 불가였다. `grep -a`(텍스트 모드)로 재검증해, 배선된 ST 에셋(ST_Door 등)은 자기 Wx 노드명을 실제로 노출하지만 `WxStateTreeTask_SetState` 를 참조하는 에셋은 전무함을 확인했다. `ST_CutsceneTrigger.uasset` 은 스키마만 있고 Wx 노드가 0개인 빈 트리다.
- **include 정리**: cpp 에서 `AWxGimmick` 캐스트는 SetState 가 유일 사용처라 함께 제거(데드 include 지양). `WxSpawner.h` 등은 TriggerSpawners 가 써서 유지.

### 계획 대비 달라진 점
- 계획대로(3파일). 추가로, 검증 과정에서 `AWxCutsceneTrigger` 가 이 태스크를 첫 채택 대상으로 **설계해 둔 주석**을 발견함(아래 후속).

### 후속 과제
- **CutsceneTrigger 복귀 경로 설계 공백(사용자 결정 필요)**: `AWxCutsceneTrigger` 는 `Playing → (재생 종료) Wx Set State → Idle` 로 설계됐으나, ST_CutsceneTrigger 에 노드가 배선된 적이 없어(빈 트리) 실제 동작한 적은 없다. Wx Set State 제거로 ST 가 권위 State 를 Idle 로 되돌릴 수단이 사라졌다(ST 는 복제 State 를 추종만 하므로 State 를 쓰는 주체가 없으면 Playing 에 고착). 대체 복귀 경로(예: 시퀀스 종료를 C++ 에서 감지해 `CommitGimmickState(Idle)`)는 사용자 결정 대기. 관련 스테일 주석: `WxCutsceneTrigger.h:27,29`, `WxCutsceneTrigger.cpp:41` — 대체안 확정 후 함께 정리.
