# WxQuest — 코드 리뷰

> 15파일짜리 소형 모듈로, 러너 소유·권위·재진입·에셋 불가지라는 설계 전제가 클래스 doc-comment에 명시돼 있고 코드가 그 전제를 대체로 지킨다 — 심각 결함은 없다. 이번 리뷰는 `WxQuest.Build.cs`·`WxQuest.uplugin`과 Public/Private 전 파일(15개)을 읽고, 컴포넌트·태스크가 기대는 엔진 계약(`UStateTreeComponent`, `FStateTreeExecutionContext`)을 UE 5.8 소스로 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 StartNextQuest 의 조립 오류가 아무 흔적 없이 삼켜진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:24-27`
- **범주**: 버그/정확성
- **문제**: 두 가지가 겹친다.
  1. 헤더 doc-comment(`Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h:27`)는 "퀘스트 컴포넌트가 없으면 … 경고를 남기고 예약하지 않는다"고 적혀 있으나, cpp 에는 `UE_LOG` 가 없다. 같은 상황에서 `SetQuestTitle`·`SetQuestObjective` 는 경고를 남기므로(`WxStateTreeTask_SetQuestTitle.cpp:26`, `WxStateTreeTask_SetQuestObjective.cpp:26`) 4종 태스크 중 이 하나만 규약을 어긴다.
  2. 그 대신 반환하는 `EStateTreeRunStatus::Failed` 는 엔진에 도달하지 못한다. 세 태스크 모두 생성자에서 `bConsideredForCompletion = false` 를 설정하는데(`WxStateTreeTask_StartNextQuest.cpp:15` 등), 엔진의 EnterState 루프는 `if (CurrentStateTasksStatus.IsConsideredForCompletion(StateTaskIndex))` 안에서만 반환값을 상태 결과에 반영한다(UE 5.8 `StateTreeExecutionContext.cpp:3873`). 즉 `Failed` 는 그대로 폐기되고 상태는 정상 진입한 것처럼 계속된다.

  결과적으로 러너 밖 조립·컴포넌트 부재 상황에서 퀘스트 체인이 다음 퀘스트로 넘어가지 않은 채, 로그도 상태 실패도 없이 조용히 끝난다. 원인 추적 단서가 0이다.
- **제안**: 다른 두 태스크와 같은 형식의 `UE_LOG(LogWxQuest, Warning, ...)` 을 추가한다. 아울러 세 태스크의 `return EStateTreeRunStatus::Failed` 는 현재 사실상 죽은 값이므로, 의미를 살리려면 `bConsideredForCompletion` 을 켜든지, 아니면 `Succeeded` 반환으로 바꾸고 진단은 로그에만 의존한다는 점을 doc-comment에 명시한다.
- **확신도**: 높음

### 2. 🟡 BP 공개 진입점이 재진입 안전 경로를 우회한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:24-25`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`
- **범주**: 설계/구조
- **문제**: 컴포넌트는 재진입 위험한 `ActivateQuest` 와 안전한 `RequestActivateQuest`(다음 틱 예약) 두 진입점을 갖는데, BP 에서 부를 수 있는 것은 `UWxQuestLibrary::StartQuest` → `ActivateQuest` 직결 경로 하나뿐이다. 이 노드가 러너 실행 콜스택 안(퀘스트 ST 노드가 호출한 BP, ST 로 구동되는 기믹 등)에서 한 번이라도 불리면 엔진은 `SetStateTreeReference` 를 거부하고(UE 5.8 `StateTreeComponent.cpp:499`) 이어지는 `StartLogic` 도 재진입 가드에 막힌다(`StateTreeComponent.cpp:183`). 그 사이 `StopLogic` 은 이미 러너를 정지시키고 `HandleStateTreeRunStatusChanged` → `ClearJournal` 로 저널까지 비운 뒤라, 진행 중이던 퀘스트가 사라지고 새 퀘스트도 시작되지 않는 복구 불가 상태로 끝난다. 호출부 규약(콜스택 밖에서만)이 코드가 아니라 doc-comment로만 강제되고, 정작 그 doc-comment(`WxQuestLibrary.h:11-16`)에는 재진입 경고가 없다.
- **제안**: `StartQuest` 를 `RequestActivateQuest` 로 라우팅해 호출 위치와 무관하게 항상 다음 틱 예약으로 만든다(하드 포인터도 `TSoftObjectPtr` 로 그대로 감싸 넘길 수 있다). 즉시 시작이 필요한 곳만 C++ 에서 `ActivateQuest` 를 직접 부르면 규약이 코드로 보장된다.
- **확신도**: 중간

### 3. 🟢 다음 틱 활성화 경로의 방어·비용
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:40-49`, `:131-134`
- **범주**: 성능/안전
- **문제**: 한 경로에 세 가지가 겹쳐 있다.
  - `:48` — `GetWorld()->GetTimerManager()` 를 널 검사 없이 역참조한다. 현재 유일한 호출부(`StartNextQuest::EnterState`)에서는 월드가 항상 유효하지만, 공개 API 이므로 월드 해체 중 호출에 무방비다.
  - 같은 프레임에 `RequestActivateQuest` 가 두 번 이상 불리면(병렬 상태에 `StartNextQuest` 가 둘, 또는 체인과 트리거 볼륨이 같은 프레임에 겹침) 예약이 합류되지 않고 타이머가 각각 쌓인다. 다음 틱에 순서대로 `ActivateQuest` 가 돌아 중간 퀘스트가 진입 → 즉시 정지되며, 그 진입 부수효과(스폰·보상 등 크로스모듈 ST 노드)만 남는다.
  - `:133` — `LoadSynchronous()` 라 퀘스트 체인 전환마다 게임 스레드 블로킹 로드가 걸린다.
- **제안**: `GetWorld()` 널 가드를 추가하고, 대기 중인 요청을 멤버 1개(`PendingQuest` + 타이머 핸들)로 유지해 마지막 요청만 살린다. 로드는 `FStreamableManager` 비동기 요청으로 바꾸면 전환 히치를 없앨 수 있다.
- **확신도**: 중간

### 4. 🟢 `HasActiveQuest()` 가 실제로 뜻하는 것은 "저널이 채워졌는가"다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:55`, `:84-87`
- **범주**: 설계/구조
- **문제**: `bHasActiveQuest` 를 `true` 로 만드는 곳은 `SetQuestTitle` 하나뿐이고 `ActivateQuest` 는 건드리지 않는다. 따라서 러너는 돌고 있으나 아직 제목 태스크가 진입하지 않은 구간, 또는 `SetQuestTitle` 태스크를 배치하지 않은 퀘스트는 이름과 반대로 "활성 퀘스트 없음"을 보고한다. 이 값을 HUD 표시 게이트로 쓰는 소비자가 있어(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:50`) 이름만 보고 "퀘스트 진행 중 판정"으로 재사용하면 어긋난다.
- **제안**: 저널 표시용이 의도라면 이름을 의미에 맞게(예: `HasJournalEntry`) 바꾸거나, 이름을 유지하려면 `ActivateQuest` 성공 시점에 플래그를 세우고 제목은 별도로 다룬다.
- **확신도**: 중간 (표시 전용 플래그라면 의도된 설계일 수 있음)

### 5. 🟢 "서버 권위"라고 적혀 있으나 저널은 복제되지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:95-102`
- **범주**: 설계/구조
- **문제**: `QuestTitle`·`Objectives`·`bHasActiveQuest` 어디에도 `Replicated` 지정자가 없고 `GetLifetimeReplicatedProps` 오버라이드도 없다. 클래스 doc-comment(`:36`)가 이를 v1 제약으로 밝히고는 있으나, 컴포넌트 사본은 클라 GameState 에도 붙으므로(`:44`) 데디케이티드 서버 구성이나 원격 클라에서는 뷰모델이 컴포넌트를 정상적으로 찾고도 영원히 빈 저널을 읽는다 — 실패가 로그 하나 없이 "퀘스트가 없는 것"처럼 보인다. 클래스·README 요약에 쓰인 "서버 권위로 관리"라는 표현은 복제가 있는 것으로 오독되기 쉽다.
- **제안**: 지금 고칠 필요는 없다. 다만 멀티플레이 확장 시 손볼 지점이므로, 원격 클라에서 컴포넌트가 발견되되 저널이 비는 구성임을 헤더 주석에 한 줄로 못 박아 두면 오독을 막는다.
- **확신도**: 낮음 (v1 싱글/리슨 호스트 전제하의 의도된 설계)

### 6. 🟢 헤더 인라인 함수 정의 (코딩 규칙 6)
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h:39`, `WxStateTreeTask_SetQuestObjective.h:43`, `WxStateTreeTask_StartNextQuest.h:38`, `WxStateTreeTask_WaitMoveToTarget.h:46`
- **범주**: 규칙 위반
- **문제**: `GetInstanceDataType()` 이 4개 헤더에서 인라인 정의돼 있다. `CLAUDE.md` 코딩 규칙 6은 인라인 함수 정의를 예외 없이 금지한다. 각 헤더 상단에 "규칙 6의 예외"라는 주석이 달려 있으나(예: `WxStateTreeTask_SetQuestTitle.h:12`), 규칙 문서 자체에는 그 예외가 없어 코드가 스스로 예외를 선언한 상태다.
- **제안**: 판단은 둘 중 하나다 — `CLAUDE.md` 규칙 6에 "StateTree 노드의 `GetInstanceDataType()`" 예외를 명문화하거나, 4개 정의를 각 `.cpp` 로 내린다. 규칙과 코드 중 한쪽만 사실이어야 한다.
- **확신도**: 낮음 (엔진 StateTree 관례를 따른 의도적 예외)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, 소비자 확인용 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`
- **규칙 대조 결과(위반 없음)**: 모듈 의존은 `WxCore` + 엔진 플러그인뿐(`WxQuest.Build.cs:11-26`), 전 소스·`Build.cs` 첫 줄 저작권 표기 존재, 람다 0건, `FORCEINLINE` 0건, `BlueprintCallable` 은 `UWxQuestLibrary` 한 곳뿐, 델리게이트 콜백 2종 모두 `Handle` 접두, `BeginPlay` 의 `Super::` 호출 확인.
- **미검토 / 한계**: 퀘스트 `UWxQuestStateTree` 에셋의 실제 저작 내용(스키마·상태 구조·태스크 배치)은 BP/에셋 영역이라 보지 않았다 — 발견 1·3의 "잘못된 조립"·"같은 프레임 중복 예약" 시나리오가 실제 에셋에서 재현되는지는 확인하지 못했다. `FWxStateTreeTask_WaitMoveToTarget` 의 매 틱 `SyncFind` 비용과 `FVector::Dist` 3D 판정은 헤더에 근거가 명시돼 있고 활성 퀘스트가 1개뿐이라 문제로 보지 않았다. StateTree 완료 판정·재진입 가드는 UE 5.8 엔진 소스로 대조했으나 실행 시나리오를 실제로 돌려보지는 않았다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 15파일 — `/module-review`로 갱신*
