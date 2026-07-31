# WxQuest — 코드 리뷰

> 러너 위임·저널·수주 경로가 한 컴포넌트에 얇게 정리돼 있고 노드 계약도 문서화가 잘 된, 전반적으로 건강한 소형 모듈이다. 소스 9파일 전부(`WxQuestComponent`, `WxQuestStateTreeNodes`, `WxQuestLibrary`, `WxQuestStateTree`, 모듈 진입점, `Build.cs`/`uplugin`)를 읽었고, 헤더 주석이 주장하는 계약 3건(`bConsideredForCompletion` 처리, `SetStateTree` 거부 조건, 에셋 레지스트리 태그 조회)은 엔진(UE 5.8) 소스로 직접 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 `StartQuest`가 에셋 교체 성공을 검증하지 않아, 실패 시 이전 퀘스트를 처음부터 재시작한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:30`(30~32행)
- **범주**: 버그/정확성
- **문제**: `StopLogic → SetStateTree → StartLogic` 세 줄이 반환값·상태 확인 없이 직진한다. 엔진의 `UStateTreeComponent::SetStateTree`는 실행 상태가 `Running`이면 경고 1줄만 남기고 **조용히 반환**한다(`StateTreeComponent.cpp:476~480`). 그리고 `StopLogic`은 ① ST 실행 콜스택 안에서 호출되면 정지를 프레임 끝으로 미루고(`StateTreeComponent.cpp:246~250`), ② `bIsRunning`이 이미 false거나 컨텍스트 요구사항 검증에 실패하면 실행 상태를 `Running`으로 남긴 채 빠져나간다(`232~235`, `258~261`). 이 중 어느 경로든 걸리면 `SetStateTree`가 거부되고 곧바로 이어지는 `StartLogic()`이 **직전 퀘스트 에셋을 루트부터 다시 시작**시킨다 — 새 퀘스트는 시작되지 않고 진행 중이던 퀘스트도 초기화되는데, 게임 로직상 아무 신호가 없다. `StartQuest`는 `UWxQuestLibrary::StartQuest`(BlueprintCallable)로 BP 어디서나 도달 가능하므로 "ST 실행 콜스택 밖에서만 호출"이라는 헤더 계약(`WxQuestComponent.h:58`)을 타입으로 강제할 수단이 없다.
- **제안**: `UWxQuestLibrary::StartQuest`를 `StartQuest` 대신 `RequestStartQuest`(다음 틱)로 위임시켜 외부 호출자에게서 재진입 위험을 통째로 제거한다. 더해 `SetStateTree` 직후 `QuestStateTree->GetStateTreeRunStatus()`나 실제 적용된 에셋을 확인해 불일치 시 `LogWxQuest` Error를 남기고 `StartLogic()`을 건너뛴다(잘못된 재시작이 침묵하는 것보다 아무것도 안 하는 편이 낫다).
- **확신도**: 중간

### 2. 🟡 저널 태스크의 `Failed` 반환은 엔진이 무시하므로 "잘못된 조립이면 Failed" 계약이 성립하지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:77`, 같은 파일 `:118`(플래그 설정은 `:68`, `:109`)
- **범주**: 버그/정확성
- **문제**: `FWxStateTreeTask_SetQuestTitle`/`SetQuestObjective`는 컴포넌트를 못 찾으면 `EStateTreeRunStatus::Failed`를 돌려주고, 헤더(`WxQuestStateTreeNodes.h:48`, `:87`)와 README는 이를 "잘못된 조립이므로 Failed"로 명시한다. 그러나 두 태스크는 `bConsideredForCompletion = false`이고, 엔진은 `EnterState` 반환 상태를 **완료 판정 대상 태스크에 한해서만** 결과에 반영한다(`StateTreeExecutionContext.cpp:3873~3881`의 `IsConsideredForCompletion` 게이트). 컴파일러도 완료 대상이 0개인 상태에는 "상태 자신" 비트만 추가할 뿐 태스크 비트를 켜지 않는다(`StateTreeCompiler.cpp:390~400`). 결과적으로 컴포넌트 부재 시 트리는 아무 일 없다는 듯 계속 진행하고, 로그조차 없어 "저널이 안 뜬다"는 증상만 남는다(`WaitMoveToTarget`은 유사 오조립에 경고를 남기는 것과 대조된다 — `:165`).
- **제안**: 두 `EnterState`의 컴포넌트 부재 분기에 `UE_LOG(LogWxQuest, Warning/Error, ...)`를 추가한다(반환값은 어차피 무시되므로 진단은 로그가 유일한 수단이다). 동시에 헤더·README의 "Failed" 문구를 실제 동작에 맞게 정정한다.
- **확신도**: 높음

### 3. 🟡 자동 탑재 스캔이 "부분 완료된 에셋 레지스트리"를 감지하지 못한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:126`(117~129행)
- **범주**: 버그/정확성
- **문제**: 경고 조건이 `QuestAssets.IsEmpty() && AssetRegistry.IsLoadingAssets()`라, 스캔이 진행 중이어도 퀘스트 에셋이 **1개라도** 잡히면 침묵한다. 퀘스트가 여러 개인 상태에서 에디터 기동 직후 PIE를 누르면 `bAutoStart` 에셋만 아직 미발견인 상황이 성립하고, 이때 자동 탑재는 조용히 누락된다("가끔 퀘스트가 안 시작된다"는 재현 어려운 증상). 쿠킹 빌드는 레지스트리가 선적재라 영향 없고 에디터 한정이지만, 침묵 오진을 없애려던 원래 의도(주석 `:125`) 자체가 반만 달성된 상태다.
- **제안**: 경고 조건에서 `IsEmpty()`를 떼고 `IsLoadingAssets()`만으로 경고하거나(발견 결과와 무관하게 "스캔 미완 중 판정함"을 알린다), 스캔 중이면 `AssetRegistry.ScanPathsSynchronous`/`WaitForCompletion`으로 확정한 뒤 판정한다.
- **확신도**: 중간

### 4. 🟢 체인 전환마다 다음 퀘스트 에셋을 게임 스레드에서 동기 로드한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:177`
- **범주**: 성능/안전
- **문제**: `HandleDeferredStartQuest`가 `QuestAsset.LoadSynchronous()`로 로드한다. `StartNextQuest`의 `NextQuest`는 소프트 참조라 실제로 미적재 상태이므로, 퀘스트 체인 전환 시점(=대체로 컷신·대화 직후 같은 눈에 띄는 순간)에 콜드 동기 로드가 발생한다. 소프트 참조 유지 사유(GC 방지) 자체는 타당하지만 로드 방식은 별개 선택지다.
- **제안**: `StartNextQuest` 진입 시점에 `FStreamableManager::RequestAsyncLoad`로 미리 요청하고, 완료 콜백에서 `StartQuest`를 호출한다(다음 틱 지연 목적도 그대로 충족된다).
- **확신도**: 중간

### 5. 🟢 아무 코드도 쓰지 않는 `DeveloperSettings` 의존
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:26`
- **범주**: 중복/복잡도
- **문제**: 모듈 전체에서 `UDeveloperSettings` 파생 클래스나 관련 헤더 사용처가 하나도 없다(모듈 소스 전수 grep 결과 `Build.cs` 한 줄이 유일한 등장). 이전 리뷰(2026-07-25) 시점의 스캐폴딩 잔재로 보이며, 그때 함께 지적된 `GameplayTags`/`StateTreeModule`/`WxCore`는 이후 실제 사용처가 생겨 해소됐다.
- **제안**: 실제 설정 클래스를 도입할 때 다시 추가하고 지금은 제거한다.
- **확신도**: 높음

### 6. 🟢 로케이터 해석·표시 헬퍼가 도메인 플러그인 3곳에 복제돼 있고 이미 서로 어긋났다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp:25`(25~28행), 같은 파일 `:32`(32~55행)
- **범주**: 중복/복잡도
- **문제**: `GetTargetText`는 `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:67`(67~90행)과 주석까지 포함해 사실상 동일하고, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:21` 부근에도 변종이 있다. 게다가 `ResolveTargetActor`는 이미 드리프트가 발생했다 — WxUI 판은 `IsValid(Target) ? Target : nullptr`로 파괴 대기 액터를 걸러내지만(`WxIndicatorStateTreeNodes.cpp:19~21`) WxQuest 판은 `Cast` 결과를 그대로 돌려준다. 세 도메인이 서로를 참조할 수 없다는 규칙상 복제 자체는 불가피한 선택이었으나, 공유 타입 `FWxActorTarget`이 이미 `WxCore`에 있으므로 헬퍼만 남겨둘 이유는 없다.
- **제안**: `WxCore`의 `WxActorTarget` 옆에 해석/표시명 헬퍼(`ResolveActor`, `#if WITH_EDITOR` 표시명)를 두고 세 모듈이 공유한다. 최소한 WxQuest 판에도 `IsValid` 가드를 맞춰 드리프트를 없앤다.
- **확신도**: 높음

### 7. 🟢 헤더 인라인 함수 정의 (CLAUDE.md 코딩 규칙 6)
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:76`(76~77행)
- **범주**: 규칙 위반
- **문제**: `HasActiveQuest()`/`GetQuestTitle()`이 클래스 본문에 정의돼 있어 "인라인 함수 정의를 금지한다"는 규칙 6에 문언상 어긋난다. 같은 파일군의 `GetInstanceDataType()`(`WxQuestStateTreeNodes.h:59`, `:98`, `:137`, `:172`)도 형태는 같으나 이쪽은 StateTree 노드의 엔진 관용구이고 프로젝트 전 도메인이 동일하게 쓰므로 별개로 본다. 실질 위험은 없고 순수 규칙 준수 항목이다.
- **제안**: 두 getter의 정의를 `.cpp`로 내린다. 엔진 관용구인 `GetInstanceDataType`까지 강제할 생각이 아니라면 규칙 6에 "ST 노드 인스턴스 데이터 타입 접근자 등 엔진 관용구는 예외" 같은 단서를 명문화하는 편이 낫다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestStateTreeNodes.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`
- **교차 확인**: 엔진 `StateTreeComponent.cpp`(정지/교체/시작 계약), `StateTreeExecutionContext.cpp`·`StateTreeTasksStatus.h`·`StateTreeCompiler.cpp`(완료 판정 마스크), `AssetData.h`(태그 조회), `Plugins/WxCore/.../WxActorTarget.h`, `Plugins/WxUI/.../WxIndicatorStateTreeNodes.cpp`(중복 대조), `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(저널 델리게이트 소비자), `Wx.uproject`
- **미검토 / 한계**: 실제 퀘스트 ST 에셋(노드 배치·전이 구성)과 이를 호출하는 레벨 스크립트 BP는 검토 대상 밖이라, 발견 1·2가 실제 조립에서 얼마나 자주 밟히는지는 확인하지 못했다(코드 경로 성립 여부만 검증). `Plugins/WxBlueprintSnapshot/Snapshots/`가 현재 저장소에 없어 BP 호출부 대조도 불가했다. 저널이 서버 권위에서만 채워지고 복제되지 않는 점은 헤더(`WxQuestComponent.h:39`, `:48`)와 README가 v1 싱글/리슨 호스트 전제로 명시한 의도된 설계이므로 발견으로 올리지 않았다 — 다만 리모트 클라이언트 HUD가 비게 되는 제약은 MP 확장 시 반드시 다시 봐야 한다.
- **참고**: 첫 줄 저작권 표기, `Wx` prefix, 델리게이트 콜백의 `Handle` prefix, `BlueprintCallable`의 BP Function Library 한정 사용, `BeginPlay`의 `Super::` 호출, "WxCore 외 Wx 플러그인 미참조" 모듈 경계, 불필요한 람다 없음 — 모두 준수한다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 9파일 — `/module-review`로 갱신*
