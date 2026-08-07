# WxQuest — 코드 리뷰

> 러너 위임·저널·수주 경로가 한 컴포넌트에 얇게 정리된 소형 모듈로, 코드 자체는 여전히 읽기 쉽고 규칙 위반도 없다. 약점은 전부 같은 자리에 몰려 있다 — 런타임에 만들어 붙인 러너에 해제 경로가 없고, 퀘스트 교체 3연타(`StopLogic`→`SetStateTree`→`StartLogic`)가 성공 여부를 아무도 확인하지 않는다. 커버리지: 소스 9파일 전부를 읽고 컴포넌트·ST 노드 cpp 를 정독했으며, 헤더가 전제하는 엔진 계약(`SetStateTree` 거부·`StartTree` 재진입 가드·재진입 `Stop` 지연·`NewObject` 동명 충돌·컴포넌트 매니저의 회수)은 UE 5.8 소스로 직접 대조했고, 소비처(`UWxViewModel_Quest`)와 실제 에셋 덤프(`ST_Quest_Main1`, `BP_QuestVolume`)까지 따라가 발현 여부를 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 런타임에 붙인 러너에 해제 경로가 없다 — 컴포넌트만 회수되면 GameState 에 남아 계속 실행된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-122`(생성 `:118-121`), `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:86`
- **범주**: 설계/구조
- **문제**: `BeginPlay` 가 `NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"))` 로 러너를 만들어 오너(GameState)에 `RegisterComponent` 하는데, 헤더에 `EndPlay`/`OnUnregister` 오버라이드가 없어(`:86` 이 유일한 오버라이드) 짝이 되는 해제가 존재하지 않는다. 러너의 오너는 GameState 이지 `UWxQuestComponent` 가 아니므로, 이 컴포넌트만 파괴되면 러너는 그대로 살아남는다.
  이 조합은 코드상 실재한다. 부착은 Experience 의 `UWxGameFeatureAction_AddComponents` 가 하고(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:157` 의 `AddComponentRequest`), 요청 핸들이 풀리면 엔진이 **살아 있는 액터에서 인스턴스를 직접 파괴한다**(`GameFrameworkComponentManager.cpp:308`, `:351`, `DestroyInstancedComponent` `:561`). 즉 GameState 는 살아 있는데 `UWxQuestComponent` 만 사라지는 상태가 성립하고, 그때 러너는 등록·Running 인 채로 퀘스트 ST 를 계속 돌린다. 노드들은 오너에서 컴포넌트를 못 찾아 진입마다 경고만 뱉고(`WxQuestStateTreeNodes.cpp:75`, `:119`), `WaitMoveToTarget` 은 영원히 틱하며, 스포너 기동·보상 지급 같은 월드 부수효과는 그대로 나간다(`ST_Quest_Main1` 의 Step2 가 `TriggerSpawnersByLocator`·`GiveRewards` 를 물고 있다).
  재주입 시의 거동은 이번에 엔진으로 확인해 두 번째 러너가 생기는 게 아니라 **더 나쁜 쪽**임을 확인했다. 이름이 `TEXT("QuestStateTree")` 로 고정이라 `NewObject` 가 동명 오브젝트를 만나면 새로 할당하지 않고 기존 것을 그 자리에서 파괴·재구성한다(`UObjectGlobals.cpp:3565-3566` "Replace an existing object without affecting the original's address or index"). 등록·실행 중인 컴포넌트가 `StopLogic` 을 거치지 않고 `BeginDestroy`(`ActorComponent.cpp:1183-1198`)로 끌려 내려간 뒤 같은 주소에 새 인스턴스가 앉는다.
  현재 실측 위험은 낮다. 이 프로젝트에서 Experience 교체는 대체로 맵 이동과 함께 오고, 그때는 GameState 가 통째로 파괴되며 러너도 같이 정리된다. 문제는 "GameState 생존 + 컴포넌트 회수" 조합이 열려 있는데 방어가 0 이라는 점이다.
- **제안**: `EndPlay`(또는 `OnUnregister`)를 오버라이드해 `QuestStateTree->StopLogic()` 후 `DestroyComponent()` 하고 포인터를 비운다. 세 줄이면 유령 실행·in-place 덮어쓰기 두 증상이 함께 사라진다.
- **확신도**: 높음(메커니즘은 엔진 코드로 확정. 현재 콘텐츠에서 실제로 밟히는 빈도는 중간 이하)

### 2. 🟡 `ActivateQuest` 가 에셋 교체 성공을 검증하지 않아, 재진입 시 퀘스트가 조용히 죽거나 되감긴다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:27-29`, 호출 경로 `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:20-26`
- **범주**: 버그/정확성
- **문제**: `StopLogic → SetStateTree → StartLogic` 세 줄이 반환값·상태 확인 없이 직진한다. ST 실행 콜스택 안에서 호출되면 세 단계가 전부 다르게 동작한다(UE 5.8 소스로 확인).
  - `StopLogic` 은 재진입이면 실행 중인 컨텍스트로 `Context.Stop()` 을 부르고(`StateTreeComponent.cpp:246-250`, 주석도 "delayed to the end of the frame"), 그 `Stop` 은 `Exec.CurrentPhase != Unset` 이면 정지를 프레임 끝으로 미루며 **`Running` 을 그대로 돌려준다**(`StateTreeExecutionContext.cpp:1707-1715`). 상태값이 안 바뀌니 `OnStateTreeRunStatusChanged` 도 발화하지 않아 저널 정리조차 이 시점엔 없다.
  - `SetStateTree` 는 인스턴스 데이터의 실행 상태로 판정하는데 그 값이 아직 `Running` 이므로 **에셋 교체가 경고 한 줄과 함께 거부된다**(`StateTreeComponent.cpp:470-481`).
  - `StartLogic` 은 `StartTree` 의 재진입 가드에 막혀 Error 로그만 남기고 반환한다(`:181-185`).

  결과적으로 기존 퀘스트만 프레임 끝에 정지되고 새 퀘스트는 세팅도 시작도 되지 않아 **활성 퀘스트 0개**로 남는다. 콜스택 밖이라도 `StopLogic` 이 컨텍스트 요구사항 검증에 실패하면(`:258-261`) 실행 상태가 `Running` 인 채 빠져나가 `SetStateTree` 가 거부되고, 바로 뒤의 `StartLogic()` 이 **직전 퀘스트를 루트부터 재시작**시킨다. 어느 쪽이든 진단은 `LogStateTree` 한두 줄이 전부고 게임 로직에는 아무 신호도 안 간다.
  헤더는 "ST 실행 콜스택 밖에서만 호출한다"고 계약을 적어 뒀지만(`WxQuestComponent.h:57`) 강제할 수단이 없다. 진입점 `UWxQuestLibrary::ActivateQuest` 는 `BlueprintCallable` 이고 BP 툴팁(`WxQuestLibrary.h:25`)에는 그 경고가 옮겨져 있지도 않다. 모듈 안에 이미 안전한 진입점(`RequestActivateQuest`, 다음 틱 예약)이 있는데 BP 진입점만 위험한 쪽을 쓴다.
- **제안**: `UWxQuestLibrary::ActivateQuest` 를 `RequestActivateQuest` 로 위임시켜 외부 호출자에게서 재진입 위험을 통째로 제거한다(한 틱 지연은 트리거 볼륨·대화 진입점에 무해하다). 즉시 활성화는 `HandleDeferredActivateQuest` 내부 전용으로 남긴다. 더해 `SetStateTree` 직후 적용된 에셋이나 실행 상태를 확인해 불일치면 `LogWxQuest` Error 를 남기고 `StartLogic()` 을 건너뛴다 — 잘못된 재시작보다 아무것도 안 하는 편이 낫다.
- **확신도**: 높음(엔진 경로는 확정. 실제 조립에 콜스택 안 호출이 있는지는 BP 그래프를 못 봐 미확인)

### 3. 🟡 예약된 활성화 요청을 추적·취소하지 않아 뒤늦은 예약이 방금 시작한 퀘스트를 덮는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:32-41`
- **범주**: 설계/구조
- **문제**: `RequestActivateQuest` 는 호출마다 별개의 next-tick 타이머를 걸고 핸들을 보관하지 않는다(`:40`). 그래서 (a) 같은 프레임에 두 번 예약되면 둘 다 발화해 뒤엣것이 앞엣것을 `StopLogic`+`StartLogic` 으로 덮고, (b) 예약이 살아 있는 사이 시작 볼륨이 `UWxQuestLibrary::ActivateQuest` 로 즉시 퀘스트를 시작하면 그 퀘스트도 다음 틱에 예약분에 덮인다. 두 경우 모두 로그가 없어 "퀘스트가 시작됐다 바로 다른 걸로 바뀌었다"로만 관측된다.
  "활성 퀘스트 동시 1개, 새 시작은 교체"라는 모델에서는 *가장 마지막 요청이 이긴다* 가 규약이어야 하는데, 지금 승자는 요청 순서가 아니라 타이머 발화 순서로 정해진다. 발견 2 의 제안대로 BP 진입점까지 `RequestActivateQuest` 로 모으면 이 경로 트래픽이 늘어나므로 함께 고치는 편이 좋다.
- **제안**: `FTimerHandle` 과 예약 대상 에셋을 멤버로 두고, 새 요청이 오면 기존 예약을 `ClearTimer` 후 덮어쓴다. `ActivateQuest` 직접 호출 시에도 대기 중 예약을 취소한다.
- **확신도**: 중간

### 4. 🟡 같은 퀘스트를 다시 활성화하면 진행이 조용히 초기화된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:17-30`
- **범주**: 버그/정확성
- **문제**: `ActivateQuest` 는 인자가 지금 돌고 있는 바로 그 에셋이어도 무조건 `StopLogic → SetStateTree → StartLogic` 을 수행한다. 진행 중이던 상태·목표·인스턴스 데이터가 전부 버려지고 트리가 루트부터 다시 시작되며 저널도 한 번 비워졌다 다시 채워진다. 아무 로그도, 아무 반환값도 없다.
  실제 수주 경로가 레벨 배치 시작 볼륨의 오버랩이라(`BP_QuestVolume` 이 `ReceiveActorBeginOverlap` 과 `QuestAsset` 변수를 갖는다), 플레이어가 퀘스트 진행 중 그 볼륨으로 되돌아오면 그대로 발현한다. `ST_Quest_Main1` 의 루트는 `Before Start`(NPC 대화 대기) → `InProgress` → Step1/Step2 순서라, Step2 에서 볼륨에 다시 들어가면 **`Before Start` 로 되감겨 NPC 대화부터 다시** 해야 하고 이미 정리한 스포너가 다시 기동된다.
  "활성 퀘스트는 동시 1개(새 시작은 교체)"라는 설계상 *다른* 퀘스트로의 교체는 의도지만, *같은* 퀘스트로의 교체까지 리셋이어야 할 이유는 문서 어디에도 없다.
- **제안**: `ActivateQuest` 진입부에 "요청 에셋이 현재 실행 중인 에셋과 같고 러너가 Running 이면 노옵" 가드를 둔다(교체 의미는 그대로 보존된다). 볼륨 쪽 1회성 처리도 가능하지만 진입점이 BP 라이브러리 하나로 모여 있으니 코드에서 막는 편이 누락이 없다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 볼륨 BP 의 이벤트 그래프에 자체 가드가 있는지는 에셋 덤프가 CDO 프로퍼티만 담고 있어 확인하지 못했다)

### 5. 🟡 `bHasActiveQuest` 가 "퀘스트 활성"이 아니라 "저널에 제목이 걸림"을 뜻한다 — 이름과 가드 양쪽에서 어긋난다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:138-149`(가드 `:140-143`), `:51-57`(유일한 set), `:59-68`(set 하지 않는 쪽), `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:75`, `:110`
- **범주**: 설계/구조
- **문제**: `bHasActiveQuest` 를 참으로 만드는 곳은 `SetQuestTitle` 하나뿐이다. `ActivateQuest` 도 `AddObjective` 도 건드리지 않는다. 그런데 이 플래그가 두 가지 다른 일을 겸한다.
  ① `ClearJournal` 의 조기 반환 가드다(`:140-143`). 목표만 등록되고 제목이 없는 상태에서 트리가 끝나면 `Objectives.Reset()` 에 도달하지 못해 목표가 저널에 그대로 남아 다음 퀘스트로 새어 나간다. 현재 에셋(`ST_Quest_Main1`)은 `SetQuestTitle` 을 `InProgress` 에 두고 목표를 전부 그 아래 스텝에 두어 밟히지 않지만, 가드가 저널 상태 전부(`QuestTitle` + `Objectives`)가 아니라 절반만 보고 있다는 사실은 그대로다.
  ② 공개 API `HasActiveQuest()` 의 값이다. `UWxViewModel_Quest` 가 이 값을 HUD 표시 여부로 쓰는데(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:50`) 표시 목적에는 맞다. 다만 이름은 "퀘스트가 실행 중인가"로 읽히는데 실제 값은 다르다 — `Before Start`(NPC 대화 대기) 구간에서는 퀘스트 ST 가 돌고 있는데 `HasActiveQuest()` 는 false 다. 이 이름을 믿고 "중복 수주 방지" 같은 판단에 쓰는 다음 소비자가 틀린 답을 받는다(발견 4 의 가드가 딱 그런 판단이다).
- **제안**: 플래그를 없애고 저널 상태에서 파생시킨다(`ClearJournal` 은 `QuestTitle` 이 비지 않았거나 `Objectives` 가 비지 않았을 때 정리). 이름도 표시 의미에 맞게 `HasJournalEntry()` 류로 바꾸고, "퀘스트 실행 중"이 정말 필요해지면 러너의 실행 상태를 따로 노출한다.
- **확신도**: 중간(①의 실제 발현은 에셋 조립에 달렸고 현재는 안 밟힌다. ②의 어긋남 자체는 코드로 확정)

### 6. 🟢 `ActivateNextQuest` 만 컴포넌트 부재 시 아무 흔적을 남기지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:221-225`
- **범주**: 버그/정확성
- **문제**: 다른 두 저널 태스크는 오너에서 컴포넌트를 못 찾으면 오너 이름까지 담아 `LogWxQuest` 경고를 남기는데(`:75-77`, `:119-120`), `ActivateNextQuest` 만 조용히 `Failed` 를 돌려준다. 이 태스크가 `Failed` 를 내면 그 상태가 실패해 퀘스트 체인이 끊기는데 원인은 어디에도 안 찍힌다 — 파일 상단 주석(`:118`)이 스스로 "오조립은 경고 로그로만 드러난다"고 적은 그 진단 수단이 여기만 없다.
- **제안**: 앞의 둘과 같은 형태로 `UE_LOG(LogWxQuest, Warning, ...)` 한 줄을 추가한다.
- **확신도**: 높음(현재 `ST_Quest_Main1` 을 포함해 어느 퀘스트 에셋도 이 노드를 쓰지 않아 체인 경로 자체가 미검증이다 — 처음 쓰는 사람이 밟을 자리다)

### 7. 🟢 빈 로케이터의 `WaitMoveToTarget` 은 경고 한 줄 뒤 영구 교착이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:161-172`, `:184-191`
- **범주**: 버그/정확성
- **문제**: 진입 시 빈 로케이터를 경고로 알리지만(`:166-169`) 그대로 `Running` 을 반환하고, `Tick` 은 대상 미해석이면 계속 `Running` 이다(`:188-191`). 빈 로케이터는 스트리밍 아웃과 달리 **나중에 해석될 여지가 없으므로** 회복 불가능한 교착이다 — 퀘스트가 그 스텝에 영원히 멈추고, 활성 퀘스트가 1개뿐이라 다른 퀘스트로 넘어갈 수도 없다. 경고는 진입 시 딱 한 번만 찍혀 나중에 로그를 봐도 눈에 안 띈다.
- **제안**: 빈 로케이터는 경고 후 `Failed` 를 반환해 그 상태를 실패시킨다. 퀘스트 트리는 대개 실패 전이를 갖고 있으므로(`ST_Quest_Main1` 에도 `Failure` 상태가 있다) 교착 대신 명시적 실패로 떨어져 훨씬 빨리 드러난다. 스트리밍 아웃(비지 않은 로케이터가 해석 안 됨)은 지금처럼 대기가 맞다.
- **확신도**: 중간(교착 자체는 확정. `Failed` 로 바꿀지 교착을 유지할지는 콘텐츠 정책 문제)

### 8. 🟢 로케이터 해석·표시 헬퍼가 도메인 4곳에 복제돼 있고 드리프트가 그대로 남아 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:24-28`, `:30-56`
- **범주**: 중복/복잡도
- **문제**: `GetTargetText`(`:30-56`)는 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:69-93` 의 `GetTargetDisplayName` 과 주석·폴백 로직(`FActorLocatorFragment` 페이로드에서 서브패스 끝 이름 추출)까지 한 글자도 다르지 않은 사본이고(반환 타입만 `FText`/`FString`), `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:20-44` 에도 같은 사본이 하나 더 있다. 이미 갈라지기도 했다 — WxWorld 판은 빈 로케이터를 "none" 이라 부르고(`:25`), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:52-61` 은 미해석을 "unloaded" 로만 표시하는 축약판이다. 같은 `FWxActorTarget` 필드를 쓰는 노드들인데 에디터 표시 문구가 셋 다 다르다.
  런타임 쪽 드리프트가 더 실질적이다. WxUI 판 `ResolveTargetActor` 는 `IsValid(Target) ? Target : nullptr` 로 파괴 대기 액터를 걸러내는데(`WxIndicatorStateTreeNodes.cpp:17-21`) WxQuest 판(`:25-28`)은 `Cast` 결과를 그대로 돌려준다. 같은 이름·같은 시그니처의 함수가 모듈마다 다른 계약을 갖는 상태다.
  세 도메인이 서로를 참조할 수 없다는 규칙상 복제 자체는 불가피했지만, 공유 타입 `FWxActorTarget` 이 이미 `WxCore` 에 있으므로 헬퍼만 각자 들고 있을 이유는 없다.
- **제안**: `WxCore` 의 `WxActorTarget` 옆에 해석/표시명 헬퍼를 두고 네 모듈이 공유한다. 당장 손댈 게 아니면 최소한 WxQuest 판에 `IsValid` 가드만이라도 맞춰 드리프트를 없앤다.
- **확신도**: 높음(복제·드리프트는 코드로 확정. 파괴 대기 액터가 `SyncFind` 로 실제 반환될 수 있는지까지는 확인하지 않았다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md` — 그리고 교차 확인용으로 엔진(UE 5.8) `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`GameFrameworkComponentManager.cpp`·`UObjectGlobals.cpp`·`ActorComponent.cpp`, 프로젝트 측 `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`Source/WxGame/MVVM/WxViewModel_Quest.cpp`, 중복 대조용 `Plugins/WxUI/.../WxIndicatorStateTreeNodes.cpp`·`Plugins/WxDialogue/.../WxDialogueStateTreeNodes.cpp`·`Plugins/WxWorld/.../WxSpawnerStateTreeNodes.cpp`, 에셋 덤프 `.claude/asset_dump/StateTrees/ST_Quest_Main1.json`·`.claude/asset_dump/Blueprints/BP_QuestStartVolume.json`
- **미검토 / 한계**: 시작 볼륨 BP 의 이벤트 그래프는 덤프가 CDO 프로퍼티만 담고 있어 확인하지 못했다 — 발견 4 의 재트리거 가드 존재 여부가 여기 달려 있다. 퀘스트 체인(`ActivateNextQuest`)은 현재 어느 에셋도 쓰지 않아 실동작이 미검증이다. 멀티플레이 실동작(전용 서버 클라이언트에서 저널이 비는 문제, `WaitMoveToTarget` 이 0번 컨트롤러만 보는 문제)은 코드상 명백하나 헤더(`WxQuestComponent.h:39`, `WxQuestStateTreeNodes.h:140`)·README 가 v1 싱글/리슨 호스트 전제로 명시한 의도된 범위라 발견으로 올리지 않았다. `Plugins/WxEditor` 의 퀘스트 에셋 팩토리는 이 모듈 밖이라 보지 않았다. 규칙 준수는 별도로 훑었고(첫 줄 저작권 표기, `Wx` prefix, 델리게이트 콜백 `Handle` prefix, `BlueprintCallable` 의 BP Function Library 한정, `BeginPlay` 의 `Super::` 호출, WxCore 외 Wx 플러그인 미참조, 불필요한 람다 없음) 위반은 없다 — 헤더의 `GetInstanceDataType()` 인라인 정의는 규칙 6 에 문언상 걸리나 전 도메인이 동일 주석과 함께 채택한 엔진 관용구라 이번에도 제외했다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 9파일 — `/module-review`로 갱신*
