# StateTree Task 파일 1태스크 1파일로 분리

## 계획

### 목표

StateTree Task 가 도메인별로 한 파일에 몰려 있어(특히 `WxGimmickStateTreeNodes` 는 14태스크 / 헤더 720줄 · 소스 1076줄) 태스크 하나를 고치려면 무관한 것들을 헤집어야 하고 태스크 추가 시 충돌 지점이 한 파일로 수렴한다. WxSave 가 이미 쓰고 있는 `WxStateTreeTask_<이름>.{h,cpp}` 규칙을 전 모듈에 적용해 태스크 = 파일이 되게 한다. 동작 변경은 없다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxWorld/.../Gimmick/WxStateTreeTask_*.{h,cpp}` | `WxGimmickStateTreeNodes` 를 14쌍으로 분리 | 신규 |
| `WxWorld/.../Gimmick/WxGimmickStateTreeNodes.{h,cpp}` | 분리 후 제거 | 삭제 |
| `WxWorld/.../Spawnable/WxStateTreeTask_{TriggerSpawnersByLocator,WaitSpawnersKilled}.{h,cpp}` | `WxSpawnerStateTreeNodes` 를 2쌍으로 분리 | 신규 |
| `WxWorld/.../Spawnable/WxSpawnerStateTreeNodes.{h,cpp}` | 분리 후 제거 | 삭제 |
| `WxWorld/.../System/WxSpawnerLibrary.{h,cpp}` | 두 스포너 태스크가 공유하는 UOL 표시명 헬퍼를 `WITH_EDITOR` static 으로 수용 | 수정 |
| `WxQuest/.../Quest/WxStateTreeTask_*.{h,cpp}` | `WxQuestStateTreeNodes` 를 4쌍으로 분리 | 신규·삭제 |
| `WxDialogue/.../WxStateTreeTask_*.{h,cpp}` | `WxDialogueStateTreeNodes` 를 3쌍으로 분리 | 신규·삭제 |
| `WxInventory/.../Inventory/WxStateTreeTask_{RefillItemCharges,GiveRewards}.{h,cpp}` | 1태스크 파일 이름만 규칙에 맞춤 | 이름 변경 |
| `WxUI/.../{Indicator,Subtitle}/WxStateTreeTask_{MarkIndicators,PrintSubtitle}.{h,cpp}` | 1태스크 파일 이름만 규칙에 맞춤 | 이름 변경 |
| `Plugins/{WxWorld,WxQuest,WxDialogue,WxUI}/README.md` | 경로 표 갱신 + 원본 상단 주석의 공통 규약 수용 | 수정 |

### 접근 방식

- **위치·이름**: 현재 폴더를 유지하고 파일명만 `WxStateTreeTask_<태스크명>` 으로. 각 헤더는 InstanceData + Task USTRUCT 한 쌍만 담고 include 도 그 태스크가 쓰는 것만 남긴다.
- **에셋 무영향**: 노드 참조는 `/Script/<Module>.<StructName>` 경로라 같은 모듈 안의 파일 이동은 에셋에 보이지 않는다. 기존 헤더는 자기 `.cpp` 외 include 처가 없어 외부 파급도 없다.
- **익명 namespace 헬퍼**: 분리하면 공유가 끊기므로 (1) 1~2줄짜리(`IsInitialEntry`, `FindGimmickComponent`, `GetQuestComponent`, `ResolveSpawner`, `FindDialogueSession` 등)는 호출부 인라인, (2) 한 태스크가 여러 번 쓰는 비자명 헬퍼(`FinishSequencePlayback`, `RefreshNpcInteraction`, Quest 표시명)는 그 struct 의 private 멤버, (3) 분리 후에도 두 파일이 진짜 공유하는 스포너 표시명 헬퍼만 `UWxSpawnerLibrary` 로 옮긴다.
- **상단 그룹 주석**: 노드 카탈로그는 태스크별 주석과 중복이라 버리고, 노드 전체가 공유하는 규약(기믹의 초기 진입 판정·재선택 정책·완료 판정, 퀘스트의 완료 비트 함정)만 각 모듈 README 로 옮긴다.
- **진행**: 모듈 단위로 끊어 빌드해 오류 원인을 좁힌다 — Spawnable → Gimmick → Quest → Dialogue → 이름 변경 → README.

---

## 완료

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxWorld/.../Gimmick/WxStateTreeTask_*.{h,cpp}` | EnableInteraction·ApplyGameplayEffectToInteractor·RespawnSpawners·EnablePlayerInput·ComponentMove·ComponentSplineMove·PlayAnimation·MoveInteractorToTarget·PlayInteractorMontage·PlayLevelSequence·PlaySound·SpawnNiagara·TriggerSpawners·SpawnActor 14쌍 | 신규 |
| `WxWorld/.../Gimmick/WxGimmickStateTreeNodes.{h,cpp}` | 분리 후 제거 | 삭제 |
| `WxWorld/.../Spawnable/WxStateTreeTask_{TriggerSpawnersByLocator,WaitSpawnersKilled}.{h,cpp}` | 2쌍 분리 | 신규 |
| `WxWorld/.../Spawnable/WxSpawnerStateTreeNodes.{h,cpp}` | 분리 후 제거 | 삭제 |
| `WxWorld/.../System/WxSpawnerLibrary.{h,cpp}` | 두 스포너 태스크가 공유하는 `GetSpawnerLocatorDisplayName`·`GetSpawnerLocatorsText` 를 `WITH_EDITOR` static 으로 수용 | 수정 |
| `WxQuest/.../Quest/WxStateTreeTask_{SetQuestTitle,SetQuestObjective,WaitMoveToTarget,ActivateNextQuest}.{h,cpp}` | 4쌍 분리, 원본 제거 | 신규·삭제 |
| `WxDialogue/.../WxStateTreeTask_{WaitDialogueCompleted,PlayDialogue,EnableNpcInteraction}.{h,cpp}` | 3쌍 분리, 원본 제거 | 신규·삭제 |
| `WxInventory/.../Inventory/WxStateTreeTask_{RefillItemCharges,GiveRewards}.{h,cpp}` | 파일명·include·generated.h 만 교체 | 이름 변경 |
| `WxUI/.../{Indicator,Subtitle}/WxStateTreeTask_{MarkIndicators,PrintSubtitle}.{h,cpp}` | 파일명·include·generated.h 만 교체 | 이름 변경 |
| `Plugins/{WxWorld,WxQuest,WxDialogue,WxUI}/README.md` | 경로 표 갱신 + 원본 상단 주석의 공통 규약 수용 | 수정 |
| `Docs/Programmer/{Gimmick_State_Authority,Spawner_Enemy_Lifecycle}.md` | 상시 참조 문서의 참조 코드 표 경로·깨진 링크 갱신 | 수정 |

### 구현·결정과 그 이유

- **파일 경계**: 태스크 = InstanceData USTRUCT + Task USTRUCT 한 쌍. include 도 그 태스크가 실제로 쓰는 것만 남겨, 원본 헤더의 전방선언 20여 개가 파일당 2~4개로 줄었다.
- **에셋 무영향**: 노드 참조는 `/Script/<Module>.<StructName>` 이라 같은 모듈 안의 파일 이동은 에셋에서 보이지 않는다. 기존 헤더는 자기 `.cpp` 외 include 처가 없어 외부 파급도 없었다.
- **헬퍼 세 갈래**: 1~2줄짜리(`IsInitialEntry`·`FindGimmickComponent`·`GetQuestComponent`·`ResolveSpawner`·`ResolveTargetActor`·`FindDialogueSession`·`GetRowText`·`GetMoveAnchor`)는 호출부 인라인. 한 태스크가 여러 번 쓰는 것(`FinishSequencePlayback`·`RefreshNpcInteraction`·Quest/Dialogue 표시명)은 그 struct 의 private 멤버(헤더 선언 / cpp 정의). 분리 뒤에도 두 파일이 진짜 공유하는 스포너 표시명만 `UWxSpawnerLibrary` 로 올렸다 — 모듈 안에 이미 있는 스포너 유틸이라 새 파일이 필요 없다.
- **공통 주석 위치**: 원본 상단 주석 중 카탈로그는 태스크별 주석과 중복이라 버리고, 노드 전체가 공유하는 규약만 README 로 옮겼다(기믹: 초기 진입 판정·재선택 정책·완료 판정 / 퀘스트: 판정 태스크 0개일 때 형제 완료 비트 상속). UOL 배열 제한·스포너 `Compile` 검증 근거처럼 한 태스크에만 걸리는 것은 그 헤더에 남겼다.
- **순수 이동 검증**: 옛 `.cpp` 와 새 파일 묶음의 코드 라인(주석·include 제외)을 다중집합으로 비교해, 차이가 위 헬퍼 치환뿐임을 확인했다. 헤더도 `UPROPERTY`·`virtual` 선언을 같은 방식으로 대조했고, 그 과정에서 TriggerSpawners·SpawnActor 의 `GetDescription` 기본 인자가 빠졌던 것을 원본대로 되돌렸다.

### 계획 대비 달라진 점

- `WxSpawnerLibrary.cpp` 에 `UniversalObjectLocator.h` 를 추가로 include 해야 했다(`ActorLocatorFragment.h` 만으로는 `FUniversalObjectLocator` 가 불완전 타입). `ApplyGameplayEffectToInteractor.cpp` 도 `GameFramework/Character.h` 가 필요했다 — 둘 다 첫 빌드에서 잡아 고쳤다.

### 후속 과제

- UOL 표시명 헬퍼가 Quest·Dialogue·UI·World 에 4벌 중복(이번 분리 이전부터) → WxCore 통합 검토.
- `WxStateTreeTask_MarkIndicators.cpp`(구 WxIndicatorStateTreeNodes) 에 익명 namespace 가 남아 있다 — 이번엔 이름만 바꾸고 내용을 건드리지 않았다.
- `Docs/Programmer/module_review_*.md` 와 옛 워크로그는 특정 커밋 시점 기록이라 옛 파일 경로를 그대로 두었다(상시 참조 문서 2개는 갱신함).

### 사후 점검(사이드이펙트)

- 에셋: `ST_Door.uasset` 등 StateTree 에셋의 참조는 `/Script/WxWorld` + struct 이름뿐이고 파일 경로가 없음을 바이너리에서 확인 — 재저장 불필요.
- 비에디터 구성: `Wx`(게임) Development 타겟 풀 빌드 성공 — `WITH_EDITOR` 가드가 온전하다(`GetActorLabel`·`Compile`·표시명 헬퍼).
- 유니티 빌드 마스킹: 새 `.cpp` 23개가 전부 adaptive unity 에서 제외돼 단독 컴파일됐음을 유니티 blob 내용으로 확인 — include 자족성이 검증됐다.
- 헤더 파일명은 프로젝트 전역에서 유일하고, IDE 프로젝트 파일(`Wx.vcxproj`)은 재생성해 옛 8개 엔트리를 제거했다.
