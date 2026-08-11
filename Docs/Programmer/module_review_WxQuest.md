# WxQuest — 코드 리뷰

> 러너 위임·저널·수주 경로가 한 컴포넌트에 얇게 정리된 소형 모듈이고 규칙 위반도 없지만, 이번엔 더 나쁜 종류의 문제가 나왔다 — ST 태스크의 완료 계약이 코드·에셋·주석 셋 사이에서 어긋나 있고, 그 어긋남을 지금은 에셋의 수동 체크박스가 가리고 있다. 커버리지: 소스 9파일 전부를 읽고 컴포넌트·ST 노드 cpp 를 정독했으며, 완료 판정 계약은 UE 5.8 StateTree 소스(`StateTreeExecutionContext.cpp`·`StateTreeTasksStatus.h`·`StateTreeCompiler.cpp`)로 대조하고 실제 에셋 `Content/Quest/ST_Quest_Main1.uasset` 의 태그 스트림을 직접 파싱해 노드별 설정까지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 `SetQuestTitle` 이 진입 즉시 `Succeeded` 를 내므로, 헤더·README 가 지시하는 배치("자식을 둔 진행 시작 상태")를 따르면 퀘스트가 시작하자마자 끝난다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:103`, 계약 서술 `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h:53`·`:57-58`·`:30-35`, `Plugins/WxQuest/README.md:28`
- **범주**: 버그/정확성
- **문제**: 헤더는 이 태스크가 "완료 없이 그 상태에 머문다"(`:53`), "완료 판정에 참여한다. 완료를 내지 않으므로 이 상태는 계속 살아 있고"(`:57`), "진행이 시작되는 상태에 한 번만 둔다 — **그 상태의 자식들이 스텝을 이룬다**"(`:54`)라고 계약을 못 박아 뒀다. 그런데 구현은 `EnterState` 에서 `EStateTreeRunStatus::Succeeded` 를 반환한다(`:103`).
  엔진에서 이 조합은 "상태 즉시 완료"다. `EnterState` 의 반환값은 그대로 그 상태의 태스크 완료 비트가 되고(`StateTreeExecutionContext.cpp:3854-3880`), 상태의 완료 상태는 마스크된 비트로 계산된다(`StateTreeTasksStatus.h:77-105`). `bConsideredForCompletion` 의 기본값은 `true`(`StateTreeTaskBase.h:34`)라 이 태스크는 마스크에 들어가고, 문서가 지시하는 배치대로 "홀로 얹힌" 상태라면 `TasksCompletion` 이 `Any` 든 `All` 든 결과가 같다 — 진입한 프레임에 그 상태가 `Succeeded` 로 완료된다. 완료 전이 탐색은 가장 얕은 활성 상태부터 돌기 때문에(`StateTreeExecutionContext.cpp:6193-6204`) 방금 진입한 자식 스텝들의 전이는 검토조차 되지 않고 부모의 완료 전이(없으면 그 위)로 빠진다. 즉 **제목만 걸리고 스텝은 한 프레임도 못 돌린다**.
  현재 이게 안 터지는 이유는 에셋이 문서와 정반대로 조립돼 있기 때문이다. `ST_Quest_Main1` 에서 `SetQuestTitle` 이 놓인 상태는 자식이 없는 통과 상태이고 `OnStateCompleted → GotoState` 전이 하나를 달고 있다(에셋 태그 스트림 파싱 결과: 해당 상태 export 에 `Children` 프로퍼티 없음, `Trigger = EStateTreeTransitionTrigger::OnStateCompleted`, `LinkType = GotoState`). 코드와 에셋은 서로 맞고 **문서만 다르다**. 다음 사람이 헤더·README 를 믿고 두 번째 퀘스트를 만들면 그 자리에서 깨진다.
- **제안**: 둘 중 하나로 통일한다. ① 문서를 정답으로 두려면 `EnterState` 가 `Running` 을 반환하도록 고친다(그러면 자식을 둔 상태에 얹는 배치가 실제로 성립하고, 기존 에셋의 통과 상태는 완료 전이가 안 도니 조립을 함께 바꿔야 한다). ② 현재 코드·에셋을 정답으로 두려면 "제목 등록 후 즉시 완료되는 통과 태스크"로 헤더(`:52-61`)·파일 상단 규약(`:30-35`)·README 를 정정한다. 어느 쪽이든 지금처럼 세 곳이 갈라진 채로 두면 안 된다.
- **확신도**: 높음(엔진 동작은 소스로 확정, 에셋 조립은 바이너리 태그 파싱으로 확인)

### 2. 🟡 `SetQuestObjective` 의 "완료 판정 제외"가 코드가 아니라 에셋 체크박스에 있다 — 빠뜨리면 스텝이 진입 즉시 완료된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:119-123`(생성자), `:139`(반환), 계약 서술 `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h:99`·`:102`
- **범주**: 설계/구조
- **문제**: 헤더는 "완료 없이 머무는 태스크라 항상 Running 이며"(`:99`), "이 태스크는 **bConsideredForCompletion=false 라** 엔진이 그 반환 상태를 결과에 반영하지 않는다"(`:102`)고 적었지만, 생성자는 `bShouldCallTick = false` 만 세팅하고 그 플래그를 건드리지 않으며(`:119-123`) `EnterState` 는 `Succeeded` 를 반환한다(`:139`). 같은 코드베이스의 동형 태스크는 코드에서 잡는다 — `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:132-135` 가 `#if WITH_EDITORONLY_DATA` 안에서 `bConsideredForCompletion = false` 를 세팅하고 `:159` 에서 `Running` 을 돌려준다.
  지금 동작하는 건 에셋이 노드마다 손으로 꺼 뒀기 때문이다(파싱 결과: `ST_Quest_Main1` 의 `SetQuestObjective` 노드 3개 모두 `bConsideredForCompletion` 이 비-기본값으로 직렬화돼 있고, 상태 6개가 `TasksCompletion = All` 로 지정돼 있다). 새 스텝 상태를 엔진 기본값(`TasksCompletion = Any`, 체크박스 켜짐)으로 만들면 `[SetQuestObjective, WaitMoveToTarget]` 조합에서 `Any` 규칙상 목표 등록 그 순간 상태가 완료되고(`StateTreeTasksStatus.h:98-104`) 대기 태스크는 한 번도 판정되지 않는다. 증상은 "목표가 번쩍이고 다음 스텝으로 넘어감"뿐이라 원인 추적이 매우 어렵다.
- **제안**: 생성자에 `#if WITH_EDITORONLY_DATA bConsideredForCompletion = false; #endif` 를 넣고 `EnterState` 는 `Running` 을 반환한다(MarkIndicators 와 동형). 그러면 에셋의 체크박스·`TasksCompletion` 설정 없이도 계약이 성립하고 헤더 서술이 사실이 된다.
- **확신도**: 높음

### 3. 🟡 런타임에 붙인 러너에 해제 경로가 없다 — 컴포넌트만 회수되면 GameState 에 남아 계속 실행된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-121`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:84`
- **범주**: 설계/구조
- **문제**: `BeginPlay` 가 `NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"))` 로 러너를 만들어 **오너(GameState)** 에 `RegisterComponent` 하는데(`:117-119`), 헤더의 오버라이드는 `BeginPlay` 하나뿐이라(`h:84`) 짝이 되는 해제가 없다. 러너의 오너는 GameState 이지 이 컴포넌트가 아니므로, GameState 는 살아 있고 `UWxQuestComponent` 만 사라지는 경로에서 러너가 그대로 남아 퀘스트 ST 를 계속 돌린다. 부착이 Experience 의 컴포넌트 주입(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:156` 의 `AddComponentRequest`)이라 요청 핸들이 풀리면 엔진이 살아 있는 액터에서 인스턴스를 파괴하는, 바로 그 조합이 열려 있다. 그때 노드들은 컴포넌트를 못 찾아 경고만 뱉지만(`WxQuestStateTreeNodes.cpp:95`, `:131`) 월드 부수효과(스포너 기동·보상)는 그대로 나간다.
  이름이 `TEXT("QuestStateTree")` 로 고정이라 재주입 시엔 `NewObject` 가 동명 오브젝트를 그 자리에서 파괴·재구성한다 — `StopLogic` 을 거치지 않은 채 끌려 내려간다.
  현재 실측 위험은 낮다(Experience 교체가 대개 맵 이동과 함께 와서 GameState 가 통째로 정리된다). 문제는 방어가 0 이라는 점이다.
- **제안**: `EndPlay`(또는 `OnUnregister`)를 오버라이드해 `QuestStateTree->StopLogic()` → `DestroyComponent()` → 포인터 정리. 세 줄이면 유령 실행과 in-place 덮어쓰기가 함께 사라진다.
- **확신도**: 높음(메커니즘 확정, 현재 콘텐츠에서 밟히는 빈도는 낮음)

### 4. 🟡 활성화 경로가 결과를 확인하지도, 경합을 정리하지도 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:27-29`(교체 3연타), `:32-41`(예약), 진입점 `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:20-26`
- **범주**: 버그/정확성
- **문제**: 두 가지가 겹쳐 있다.
  ① `StopLogic → SetStateTree → StartLogic`(`:27-29`)이 반환값·상태 확인 없이 직진한다. 엔진에서 `SetStateTree` 는 실행 상태가 `Running` 이면 경고 한 줄과 함께 **교체를 거부**하고(`StateTreeComponent.cpp:470-482`), 재진입 호출이면 `StopLogic` 이 정지를 프레임 끝으로 미루며(`:246-250`) `StartTree` 는 재진입 가드에 막힌다(`:181-185`). 어느 쪽이든 결과는 "새 퀘스트가 안 켜졌거나, 직전 퀘스트가 루트부터 재시작"인데 게임 로직에는 아무 신호도 안 간다. 헤더는 "ST 실행 콜스택 밖에서만 호출한다"고 적었지만(`WxQuestComponent.h:55`) 진입점 `UWxQuestLibrary::ActivateQuest` 는 `BlueprintCallable` 이고 BP 툴팁(`WxQuestLibrary.h:24`)에 그 경고가 옮겨져 있지도 않다.
  ② `RequestActivateQuest` 는 호출마다 별개의 next-tick 타이머를 걸고 핸들을 보관하지 않는다(`:40`). 같은 프레임에 두 번 예약되면 둘 다 발화해 뒤엣것이 앞엣것을 덮고, 예약이 살아 있는 사이 트리거 볼륨이 즉시 활성화하면 그 퀘스트도 다음 틱에 덮인다. "동시 1개, 새 시작이 교체"라는 모델에서 승자가 요청 순서가 아니라 타이머 발화 순서로 정해진다.
- **제안**: BP 진입점을 `RequestActivateQuest` 로 위임해 외부 호출자에게서 재진입 위험을 제거하고(한 틱 지연은 볼륨·대화 진입점에 무해), `FTimerHandle` + 예약 대상을 멤버로 두어 새 요청이 기존 예약을 `ClearTimer` 로 덮게 한다. 더해 `SetStateTree` 뒤 적용 결과를 확인해 불일치면 `LogWxQuest` Error 를 남기고 `StartLogic()` 을 건너뛴다 — 잘못된 재시작보다 아무것도 안 하는 편이 낫다.
- **확신도**: 높음(엔진 경로 확정. 실제 BP 조립에 콜스택 안 호출이 있는지는 미확인)

### 5. 🟡 같은 퀘스트를 다시 활성화하면 진행이 조용히 초기화된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:17-30`
- **범주**: 버그/정확성
- **문제**: `ActivateQuest` 는 인자가 지금 돌고 있는 바로 그 에셋이어도 무조건 정지·교체·재시작을 수행한다. 진행 상태·목표·인스턴스 데이터가 전부 버려지고 트리가 루트부터 다시 시작되며 저널도 한 번 비워졌다 다시 채워진다. 반환값도 로그도 없다. 수주 경로가 레벨 배치 볼륨의 오버랩이라(`Content/Quest/BP_QuestVolume.uasset`) 플레이어가 진행 중 그 볼륨으로 되돌아오면 그대로 발현하고, 이미 지나온 스텝의 스포너·보상 구간이 다시 돈다. "다른 퀘스트로의 교체"는 설계 의도지만 "같은 퀘스트로의 교체까지 리셋"이어야 할 근거는 어디에도 없다.
- **제안**: `ActivateQuest` 진입부에 "요청 에셋 == 현재 실행 중 에셋 && 러너가 Running 이면 노옵" 가드를 둔다. 볼륨 BP 쪽 1회성 처리로도 막을 수 있지만 진입점이 라이브러리 하나로 모여 있으니 코드에서 막는 편이 누락이 없다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 볼륨 BP 이벤트 그래프에 자체 가드가 있는지는 확인하지 못했다)

### 6. 🟢 `ActivateNextQuest` 만 컴포넌트 부재 시 아무 흔적을 남기지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:243-246`
- **범주**: 버그/정확성
- **문제**: 다른 두 저널 태스크는 오너 이름까지 담아 경고를 남기는데(`:95-96`, `:131-132`) 이 태스크만 조용히 `Failed` 를 돌려준다. `Failed` 는 그 상태를 실패시켜 퀘스트 체인을 끊는데 원인은 어디에도 안 찍힌다 — 이웃 태스크의 주석(`WxQuestStateTreeNodes.h:102`)이 "오조립은 경고 로그로만 드러난다"고 적은 그 진단 수단이 여기만 없다.
- **제안**: 앞의 둘과 같은 형태로 `UE_LOG(LogWxQuest, Warning, ...)` 한 줄 추가.
- **확신도**: 높음(현재 어느 퀘스트 에셋도 이 노드를 쓰지 않아 체인 경로 자체가 미검증이다 — 처음 쓰는 사람이 밟을 자리다)

### 7. 🟢 빈 로케이터의 `WaitMoveToTarget` 은 경고 한 줄 뒤 영구 교착이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:176-189`, `:210-219`
- **범주**: 버그/정확성
- **문제**: 진입 시 빈 배열·빈 로케이터를 경고로 알리지만(`:176-187`) 그대로 `Running` 을 반환하고, `Tick` 은 대상 미해석이면 계속 `Running` 이다(`:210-219`). 빈 로케이터는 스트리밍 아웃과 달리 나중에 해석될 여지가 없으므로 회복 불가능한 교착이다 — 퀘스트가 그 스텝에 영원히 멈추고, 활성 퀘스트가 1개뿐이라 다른 퀘스트로 넘어갈 수도 없다. 경고는 진입 시 한 번뿐이라 나중에 로그를 봐도 눈에 안 띈다.
- **제안**: 빈 배열·빈 로케이터는 경고 후 `Failed` 를 반환해 명시적으로 실패시킨다(퀘스트 트리는 실패 전이를 갖고 있다). 비어 있지 않은데 해석만 안 되는 스트리밍 아웃은 지금처럼 대기가 맞다.
- **확신도**: 중간(교착 자체는 확정. `Failed` 로 바꿀지는 콘텐츠 정책 문제)

### 8. 🟢 `SetQuestObjective` 는 재선택 방어가 없어 같은 상태가 다시 선택되면 목표가 지워졌다 다시 붙는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:119-123`(방어 없음) ↔ `:166-170`(같은 상태의 `WaitMoveToTarget` 은 `bShouldStateChangeOnReselect = false`)
- **범주**: 설계/구조
- **문제**: `bShouldStateChangeOnReselect` 의 기본값은 `true` 라, 이미 활성인 상태를 타깃으로 하는 전이(Sustained)가 오면 엔진이 이 태스크의 `ExitState` → `EnterState` 를 다시 호출한다(`StateTreeExecutionContext.cpp:3838-3840`, `:4029-4031`). 결과는 `RemoveObjective` → `AddObjective` 로 저널 브로드캐스트 2회 + 목표가 목록 맨 뒤로 이동이다. 같은 상태에 얹힌 대기 태스크는 진행이 끊기지 않도록 이미 `false` 로 막아 뒀고(`:166-170`), 동형인 `MarkIndicators` 도 표시 깜빡임을 이유로 `false` 다(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:130`). 표시 태스크만 방어가 빠져 비대칭이다.
- **제안**: 생성자에 `bShouldStateChangeOnReselect = false` 를 추가한다.
- **확신도**: 중간(재선택 전이가 현재 에셋에 있는지는 확인하지 않았다)

### 9. 🟢 로케이터 해석·표시 헬퍼가 도메인마다 복제돼 있고 런타임 계약이 이미 갈라졌다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:25-28`, `:32-78`
- **범주**: 중복/복잡도
- **문제**: `GetTargetDisplayName`(`:32-55`)은 `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:21-44` 의 사본과 주석·폴백 로직(`FActorLocatorFragment` 페이로드에서 서브패스 끝 이름 추출)까지 동일하고 빈 로케이터 문구만 다르다(`unset` vs `none`). `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp` 에도 같은 계열이 하나 더 있다.
  더 실질적인 건 런타임 드리프트다. WxUI 판 `ResolveTargetActor` 는 `IsValid(Target) ? Target : nullptr` 로 파괴 대기 액터를 걸러내는데(`:17-22`), WxQuest 판(`:25-28`)과 WxWorld 판(`:14-17`)은 `Cast` 결과를 그대로 돌려준다. 같은 이름·같은 시그니처 함수가 모듈마다 다른 계약을 갖는다.
- **제안**: 도메인 간 참조 금지 규칙상 각자 들고 있는 것 자체는 불가피하니, 해석·표시명 헬퍼를 `WxCore` 로 올려 넷이 공유한다. 당장 손대지 않을 거라면 최소한 WxQuest 판에 `IsValid` 가드만이라도 맞춘다.
- **확신도**: 높음(복제·드리프트는 코드로 확정. 파괴 대기 액터가 `SyncFind` 로 실제 반환되는지까지는 확인하지 않았다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md` — 교차 확인용으로 엔진(UE 5.8) `StateTreeExecutionContext.cpp`·`StateTreeTasksStatus.h`·`StateTreeTaskBase.h`·`StateTreeTypes.h`·`StateTreeCompiler.cpp`·`StateTreeComponent.cpp`, 프로젝트 측 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`·`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, 중복 대조용 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp`·`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp`, 그리고 `Content/Quest/ST_Quest_Main1.uasset` 의 name table·태그 스트림
- **미검토 / 한계**: 에셋 검증은 `.uasset` 바이너리의 프로퍼티 태그를 직접 파싱한 것이라 상태 계층·전이 대상까지 완전히 복원하지는 못했다 — 발견 1·2 의 "에셋이 이렇게 조립돼 있다" 부분은 에디터에서 눈으로 한 번 더 확인하는 편이 좋다. BP(`BP_QuestVolume`) 이벤트 그래프는 범위 밖이라 발견 5 의 재트리거 가드 존재 여부는 미확인이다. 퀘스트 체인(`ActivateNextQuest`)은 현재 어느 에셋도 쓰지 않아 실동작이 미검증이다. 멀티플레이(전용 서버 클라이언트에서 저널이 비는 문제, `WaitMoveToTarget` 이 0번 컨트롤러만 보는 문제)는 헤더(`WxQuestComponent.h:37`, `WxQuestStateTreeNodes.h:142`)·README 가 v1 싱글/리슨 호스트 전제로 명시한 의도된 범위라 발견으로 올리지 않았다. 규칙 준수는 별도로 훑었고(첫 줄 저작권, `Wx` prefix, 델리게이트 콜백 `Handle` prefix, `BlueprintCallable` 의 BP Function Library 한정, `BeginPlay` 의 `Super::` 호출, WxCore 외 Wx 플러그인 미참조, 람다 없음) 위반은 없다 — 헤더의 `GetInstanceDataType()` 인라인 정의는 규칙 6 에 문언상 걸리나 전 도메인이 동일 주석(`WxQuestStateTreeNodes.h:14`)과 함께 채택한 엔진 관용구라 이번에도 제외했다.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 9파일 — `/module-review`로 갱신*
