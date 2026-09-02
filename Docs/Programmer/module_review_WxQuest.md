# WxQuest — 코드 리뷰

> 14파일 규모의 작고 응집도 높은 모듈이다. 플러그인 의존 규칙·네이밍·Handle 접두사·인라인 금지 등 프로젝트 코딩 규칙 위반은 하나도 없고, 러너 소유와 저널 정리 경로가 한 곳으로 수렴하도록 잘 정리돼 있다. 남은 문제는 대부분 「데이터 저작이 밟을 수 있는 경계 상황」과 「v1 싱글 전제」다. 이번 리뷰는 `Public`/`Private` 전 소스 14파일과 `Build.cs`·`uplugin` 을 전부 읽고, 검증이 필요한 지점은 UE 5.8 엔진 소스(`StateTreeComponent.cpp`, `StateTreeExecutionContext.cpp`)와 소비자 측(`Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`)까지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 다음 틱 활성화 예약을 취소·통합할 수 없어 뒤늦은 예약이 방금 시작한 퀘스트를 덮는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:39`
- **범주**: 버그/정확성
- **문제**: `RequestActivateQuest` 가 `SetTimerForNextTick` 의 `FTimerHandle` 을 보관하지 않아 예약은 쌓이기만 하고 취소·중복 제거 수단이 없다. 실패 시나리오 둘:
  - `FWxStateTreeTask_StartNextQuest` 는 생성자에서 `bShouldStateChangeOnReselect` 를 끄지 않는다(`Private/Quest/WxStateTreeTask_StartNextQuest.cpp:9`). 같은 상태가 재선택되면 `EnterState` 가 다시 돌아 예약이 하나 더 쌓이고, 다음 틱에 `ActivateQuest` 가 두 번 돌아 방금 켠 퀘스트를 Stop→Start 로 한 번 더 재시작한다(저널도 비웠다 다시 채워진다).
  - 체인 예약이 걸린 프레임에 레벨 트리거가 `UWxQuestLibrary::StartQuest` 로 다른 퀘스트를 시작하면, 다음 틱에 예약분이 그 퀘스트를 조용히 교체한다. 플레이어가 방금 수주한 퀘스트가 로그 한 줄 없이 사라진다.
- **제안**: `FTimerHandle` 을 멤버로 들고 예약 전에 기존 것을 `ClearTimer` 하거나, 대기 요청을 소프트 참조 1개로 들고 다음 틱에 한 번만 소비한다. `ActivateQuest` 진입 시에도 대기 예약을 버려 즉시 호출이 우선하게 한다.
- **확신도**: 중간

### 2. 🟡 SetQuestTitle 이 살아 있는 목표를 통째로 지우고, 되살릴 경로가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:50`
- **범주**: 설계/구조
- **문제**: `SetQuestTitle` 이 `Objectives.Reset()` 으로 목록 전체를 비운다. 목표는 `FWxStateTreeTask_SetQuestObjective` 가 `EnterState` 에서만 걸고 `ExitState` 에서 걷어가는 구조라, 이미 진입을 끝낸 태스크의 목표가 지워지면 복구 경로가 없다. 상위 상태가 목표를 건 뒤 하위 상태에서 제목을 다시 설정하거나(챕터 전환 등) 트리 조립상 제목 태스크가 나중에 도는 상태가 하나라도 있으면 상위 목표가 사라진다. 뒤늦은 `RemoveObjective` 는 핸들을 못 찾고 조용히 반환하므로(`WxQuestComponent.cpp:69`) 경고조차 남지 않는다.
- **제안**: 제목 설정과 목표 초기화를 분리한다 — `SetQuestTitle` 은 제목만 갱신하고 목표 비우기는 러너 종료 경로(`ClearJournal`)에만 맡긴다. 초기화를 유지해야 한다면 남은 목표가 있는 상태에서 비울 때 `LogWxQuest` 경고를 남긴다.
- **확신도**: 중간

### 3. 🟡 저널 상태에 리플리케이션이 없어 비-권위 사본은 영구히 빈 저널이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:95`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:41`
- **범주**: 설계/구조
- **문제**: `QuestTitle`·`Objectives`·`bHasActiveQuest` 에 `GetLifetimeReplicatedProps` 가 전혀 없고 `OnJournalChanged` 도 권위에서만 발화한다. 반면 컴포넌트 자체는 사이드 구분 없는 주입 목록(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:151`)으로 클라 GameState 에도 붙고, 뷰모델 리졸버는 `GameState->FindComponentByClass<UWxQuestComponent>()` 로 그 사본을 그대로 잡는다(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:68`). 데디 서버 구성에서 클라 퀘스트 HUD 는 항상 비어 있게 된다. 도달 판정도 `UGameplayStatics::GetPlayerController(Owner, 0)` 고정이라 데디 서버에선 임의의 접속자 1명만 추적한다.
- **제안**: v1 범위를 넘길 때 저널을 `DOREPLIFETIME` + `OnRep` 브로드캐스트로 올리거나 PlayerState 단위로 옮기고, 대상 폰은 태스크 파라미터/컨텍스트로 받게 바꾼다. 그 전까지는 최소한 비-권위 사본에서 저널 조회 API 가 불릴 때 진단을 남겨 「HUD 가 왜 비었나」를 빨리 짚게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — README 와 `WxQuestComponent.h:36,44` 주석에 v1 싱글/리슨 호스트 전제로 명시돼 있다.

### 4. 🟡 런타임 생성 러너의 수명 정리가 없고 고정 이름이라 재생성이 위험하다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:105`
- **범주**: 설계/구조
- **문제**: `QuestStateTree` 는 `Owner`(GameState)를 아우터로 `NewObject` 후 `RegisterComponent` 되므로 `UWxQuestComponent` 의 하위가 아니라 액터의 형제 컴포넌트다. 그런데 `EndPlay`/`OnUnregister` 오버라이드가 없어 컴포넌트가 회수돼도 러너는 GameState 에 남아 계속 돈다 — 퀘스트 태스크의 월드 부수효과(스폰·보상)만 이어지고 저널을 갱신할 주체는 이미 사라진 상태가 된다. 또 이름이 `TEXT("QuestStateTree")` 로 고정이라 같은 GameState 에서 컴포넌트가 회수 후 재주입되면 `NewObject` 가 아직 등록된 채인 기존 오브젝트를 같은 이름으로 덮어쓴다. 현재는 Experience 가 월드당 1회만 확정되고(`Source/WxGame/Framework/WxExperienceManagerComponent.cpp:104`) 해제도 `EndPlay` 에서만 일어나 실제로 밟히지 않지만, 컴포넌트 회수 경로 자체는 이미 구현돼 있다(`WxGameFeatureAction_AddComponents.cpp:77` 의 `ContextHandles.Remove` 가 `FComponentRequestHandle` 을 해제한다).
- **제안**: `EndPlay`(또는 `OnUnregister`)에서 `QuestStateTree->DestroyComponent()` 로 러너를 회수하고, 이름은 고정 대신 `NAME_None` 으로 두거나 생성 전 기존 인스턴스를 정리한다.
- **확신도**: 중간

### 5. 🟢 bConsideredForCompletion=false 라 세 태스크의 Failed 반환이 전파되지 않는다 — 주석과 어긋난다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:25`, `Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:28`, `Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:28`
- **범주**: 버그/정확성
- **문제**: 세 태스크 모두 생성자에서 `bConsideredForCompletion = false` 로 두고(각 cpp 14~16행) `EnterState` 에서 `EStateTreeRunStatus::Failed` 를 돌려준다. 엔진은 완료 판정 대상이 아닌 태스크의 상태를 `Result` 에 반영하지 않으므로(UE 5.8 `StateTreeExecutionContext.cpp:3873`), 이 Failed 는 어디에도 전달되지 않고 상태는 그대로 진행한다. `Public/Quest/WxStateTreeTask_StartNextQuest.h:27` 의 "예약 없이 Failed 로 끝난다" 는 서술이 실제 동작과 다르다. 게다가 StartNextQuest 만 형제 태스크와 달리 경고 로그가 없어(`WxStateTreeTask_StartNextQuest.cpp:23`) 체인이 끊겨도 아무 흔적이 남지 않는다.
- **제안**: 반환값에 기대지 말고 StartNextQuest 에도 나머지 둘과 같은 `LogWxQuest` 경고를 넣고, 헤더 주석을 실제 동작(상태를 끝내지 않고 아무 일도 하지 않음)으로 정정한다.
- **확신도**: 높음

### 6. 🟢 목표 지점 도달 대기에 탈출구가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:19`
- **범주**: 버그/정확성
- **문제**: 로케이터가 비어 있으면 진입 시 경고 한 줄만 남기고 `Running` 을 돌려준다. 이후 `Tick` 은 `SyncFind` 가 계속 null 이라 영원히 `Running` 이고, 퀘스트가 그 상태에 갇힌 채 복구 수단이 없다. 이 태스크는 완료 판정 대상이므로 진입 실패를 그대로 전파할 수 있는데도 쓰지 않고 있다.
- **제안**: 빈 로케이터는 조립 오류이므로 `EnterState` 에서 `Failed` 를 돌려 상태를 끝내거나, 대기 상한(타임아웃) 파라미터를 둔다.
- **확신도**: 중간

### 7. 🟢 Build.cs 에 쓰지 않는 GameplayTags 의존
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 어디에서도 GameplayTag 타입을 쓰지 않는다.
- **제안**: 제거한다. 같은 줄에서 `WxCore`(20행)도 `#if WITH_EDITOR` 블록의 `FWxLocatorUtils` 에서만 쓰이므로 `PrivateDependencyModuleNames` 로 내릴 수 있다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_*.h`(4), `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`
- **참고로 확인한 모듈 밖 파일**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp/.h`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, UE 5.8 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`StateTreeTaskBase.h`
- **규칙 준수 확인 결과**: 소스 14파일 전부 첫 줄 Copyright 일치, 람다 0건, 인라인 정의는 `GetInstanceDataType()` 뿐이며 4개 헤더 모두 예외 사유 주석을 달았다. `BlueprintCallable` 은 `UWxQuestLibrary::StartQuest` 한 곳뿐(BP Function Library — 허용), 델리게이트 콜백은 `HandleStateTreeRunStatusChanged`·`HandleDeferredActivateQuest` 로 `Handle` 접두사를 지켰고, `BeginPlay` 는 `Super::` 를 호출한다. 플러그인 의존은 `WxCore` 외 Wx 플러그인이 없다. 즉 **규칙 위반 발견 0건**이다.
- **미검토 / 한계**: 실제 퀘스트 StateTree 에셋(BP/데이터)의 조립 형태는 범위 밖이라, 2·5·6번이 현행 에셋에서 실제로 밟히는지는 확인하지 못했다 — 코드상 도달 가능성만 근거로 적었다. `FUniversalObjectLocator::SyncFind` 의 매 틱 비용은 경로 조회 수준이라 판단해 프로파일링하지 않았다. 리플리케이션 관련 판단(3번)은 데디 서버 실행 검증 없이 정적 분석만으로 내렸다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 14파일 — `/module-review`로 갱신*
