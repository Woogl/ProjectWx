# module-review 확신도 높음 이슈 해결

## 계획

### 목표
`module-review` 스킬이 생성한 `Docs/Programmer/module_review_*.md` 10개 문서에서 확신도 '높음' 발견을 골라 해결한다. 총 10건 중 WxWorld #1(사용자 직접 판단 예정)·WxSave #2(현행 유지)는 제외하고 나머지 8건을 처리한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../WxInventoryManagerComponent.cpp` | `PreReplicatedRemove`에서 통지 전 `Entry.StackCount=0`으로 내려 클라 총량(NewCount) 과다 계산 버그 수정 | 수정 |
| `Plugins/WxUI/.../MVVM/WxViewModel_InteractionList.cpp` | `Deinitialize` 말미에 `Super::Deinitialize()` 추가 | 수정 |
| `Plugins/WxCore/.../Public/WxSavable.h` | 주석 `GetWxSaveId()` → `GetSaveId()` | 수정 |
| `Plugins/WxSave/.../Public/WxSaveGame.h` | 주석 `IWxSavable::GetWxSaveId()` → `GetSaveId()` | 수정 |
| `Source/WxGame/.../WxAbility_UseItem.cpp` | 주석의 틀린 "(WxAbility_Interact 와 동일)" 제거 | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.cpp` | `GetDescription` bool 포맷 0/1 → 삼항 true/false | 수정 |
| `Plugins/WxAI/.../WxBTTask_Wander.cpp` | 미사용 `#include "NavigationSystem.h"` 제거 | 삭제 |
| `Plugins/WxInventory/.../WxInventory.Build.cs` | 미사용 `DeveloperSettings` 의존성 제거 | 삭제 |
| `Plugins/WxSound/.../System/WxMusicSubsystem.cpp` | 중복 `PlayerStateTags.Reset()`를 ASC 부재 `else`로 이동 | 수정 |

### 접근 방식
- **WxInventory #1 (실질 버그)**: FastArray `PreReplicatedRemove`는 엔트리가 배열에서 제거되기 전에 호출된다. `NotifyStackChangedFromList`의 총량 재계산(`GetTotalItemCountByDefinition`=StackCount 합산)이 제거분을 포함해 과다 계산되는 것이 원인. 재계산 통지 전에 해당 엔트리 `StackCount`를 0으로 내려 합산에서 제외 → 서버(제거 후 재계산)와 동일한 사후 총량 발행. 동일 배치의 같은 Def 다중 제거도 누적 정확. 엔트리는 콜백 직후 제거되어 mutate 무해.
- **나머지 7건**: 주석 정정·bool 포맷·미사용 include/의존성 제거·중복 Reset 정리 등 동작 영향 최소 또는 클라 표기/에디터 전용.

### 검증
WxEditor(Development) 타겟 빌드로 컴파일 확인. 실패 시 `build-doctor`.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../WxInventoryManagerComponent.cpp` | `PreReplicatedRemove`에서 통지 전 `Entry.StackCount=0` → 클라 총량 과다 계산 버그 수정 | 수정 |
| `Plugins/WxUI/.../MVVM/WxViewModel_InteractionList.cpp` | `Deinitialize` 말미 `Super::Deinitialize()` 추가 | 수정 |
| `Plugins/WxCore/.../Public/WxSavable.h` | 주석 `GetWxSaveId()` → `GetSaveId()` | 수정 |
| `Plugins/WxSave/.../Public/WxSaveGame.h` | 주석 `IWxSavable::GetWxSaveId()` → `GetSaveId()` | 수정 |
| `Source/WxGame/.../WxAbility_UseItem.cpp` | 틀린 "(WxAbility_Interact 와 동일)" 주석 제거 | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.cpp` | `GetDescription` bool 포맷 0/1 → 삼항 true/false | 수정 |
| `Plugins/WxAI/.../WxBTTask_Wander.cpp` | 미사용 `#include "NavigationSystem.h"` 제거 | 삭제 |
| `Plugins/WxInventory/.../WxInventory.Build.cs` | 미사용 `DeveloperSettings` 의존성 제거 | 삭제 |
| `Plugins/WxSound/.../System/WxMusicSubsystem.cpp` | 중복 `PlayerStateTags.Reset()`를 ASC 부재 `else`로 이동 | 수정 |

### 구현·결정과 그 이유
- **WxInventory 버그 수정 방식**: 리뷰 제안(`GetTotal - LastObservedCount` 또는 명시 NewCount 시그니처)보다 `Entry.StackCount=0` 선-하향을 택했다. 기존 재계산 경로를 그대로 두면서 동일 배치의 같은 Def 다중 제거까지 누적 정확하게 처리되고, 새 API/시그니처 추가 없이 최소 변경으로 끝난다. 엔트리는 콜백 직후 배열에서 제거되어 mutate가 무해함을 근거로 삼았다.
- **주석 2건(WxCore/WxSave)만 정정**: `GetWxSaveId`는 worklog·분석문서에도 있으나 이력/파생 문서라 코드 계약 주석(WxSavable.h, WxSaveGame.h)만 손봤다.
- **WxSound Reset**: 완전 제거 대신 ASC 부재 `else`로 옮겨, ASC 없을 때 낡은 스냅샷을 비우는 원래 역할을 보존했다.

### 계획 대비 달라진 점
- 계획대로. WxWorld #1(규칙 7)·WxSave #2(Handle 개명)는 사용자 결정에 따라 미포함.

### 검증
- WxEditor(Development) 빌드 성공 (`Result: Succeeded`, exit 0). 관련 8개 모듈 전부 재컴파일·링크 통과. 로그: `.claude/skills/build-doctor/logs/build_2026-07-22_*.log`.

### 후속 과제
- **WxWorld #1**(BlueprintCallable 규칙 7 위반, 확신도 높음): 사용자가 직접 코드 확인 후 처리 방향 결정 예정.
- 확신도 중간·낮음 발견들(각 module_review 문서)은 이번 범위 밖 — 필요 시 별도 처리.
