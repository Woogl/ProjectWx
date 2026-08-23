# WxWorld — 코드 리뷰

> 여전히 건강한 모듈이다 — 서버 권위·복제 추종·세이브 복원 모델이 doc-comment 와 구현이 일치하고, CLAUDE.md 규칙 위반(저작권 줄·Wx Prefix·`BlueprintCallable`·인라인 정의·`Super::` 누락·플러그인 경계)은 50 파일 전부에서 발견되지 않았다. 이번 리뷰는 최근 재편된 Device 계열(`AWxDevice`·`UWxDeviceStateTreeComponent`, `AWxTriggerDevice` 는 삭제되어 직전 리뷰의 🟡 1건은 소멸)과 상호작용 스캐너·대기 태스크·스포너를 cpp 까지 깊게 보고, 연출 태스크 12종은 수명주기·권위 게이트·인스턴스 데이터 GC 안전성 위주로 훑었으며, StateTree 동작은 UE 5.8 엔진 소스로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 「전이 없음」을 「같은 상태 재진입」으로 오판해 클라만 현재 상태를 재진입한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:77`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:152-155`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:135-147`
- **범주**: 버그/정확성 (권위 모델)
- **문제**: 권위 상호작용은 `NotifyInteractionPending()` 으로 플래그를 세운 뒤 발행하고, 다음 `PublishAuthorityState` 가 「플래그가 섰는데 StateTag 가 그대로」를 자기 전이(재진입)로 간주해 `Multicast_ReenterState` 를 보낸다. 그러나 **발행이 아무 전이에도 닿지 않은 경우가 같은 관측 결과를 낸다.** UE 5.8 `TStateTreeStrongExecutionContext::BroadcastDelegate`(`StateTreeAsyncExecutionContext.cpp:101-124`)는 Dispatcher 가 비면 "Nothings binds, not an error" 로 `true` 를 답하고, 유효해도 `FStateTreeDelegateActiveListeners::BroadcastDelegate` 가 리스너의 StateID 가 현재 ActiveStates 에 없으면 아무것도 실행하지 않는다 — 그런데 `BroadcastInteractionDelegate` 는 반환값조차 버린다(`WxDevice.cpp:154`). 구체 시나리오: ① '상호작용 켜기' 는 상태를 떠나도 되돌리지 않으므로(`Public/Interaction/WxStateTreeTask_EnableInteraction.h:39-40`) 앞 상태가 켜 둔 상호작용을 다음 상태(문의 Opening 등)에서 누르면 `CanInteract()` 게이트를 통과하지만 그 상태의 전이는 이 발행자를 듣지 않는다. ② 남의 트리가 `SetInteractionEnabled(true)` 로 켠 장치(`WxDevice.cpp:47-51`)는 Dispatcher 자체가 비어 있다. ③ 전이는 듣고 있으나 전이 조건이 서버에서 실패한 경우. ④ 복원 수렴 중(`bFollowRestoredState`)에 눌리면 그 구간엔 `PublishAuthorityState` 가 돌지 않아 플래그가 남았다가 수렴 직후 터진다. 결과는 서버는 가만히 있는데 클라만 `RequestState(StateTag)` 로 현재 상태를 재선택하고, 엔진이 Sustained 재선택에서 `bShouldStateChangeOnReselect` 인 태스크의 `EnterState` 를 다시 부르므로(`StateTreeExecutionContext.cpp:3838-3852`) 기본값 true 인 PlayAnimation·PlaySound·SpawnNiagara·PlayInteractorMontage 가 클라에서만 재생된다 — 눈에 보이는 디싱크다. `NotifyDeviceInteracted` 의 주석(`WxDevice.cpp:116-117`)이 이벤트 경로에서 피하려 한 바로 그 현상이 자기 상호작용 경로엔 열려 있다.
- **제안**: 추론 대신 트리의 사실을 읽는다. `UWxDeviceStateTreeComponent::TickComponent` 에서 `Super::TickComponent` 전에 `FStateTreeReadOnlyExecutionContext::GetStateChangeCount()`(5.8 확인)를 읽어 두고, 틱 뒤 `ActiveTag == StateTag` 이면서 카운트가 올랐을 때만 멀티캐스트한다 — 엔진은 같은 상태 재선택을 포함한 모든 상태 선택에서 이 카운터를 올린다(`StateTreeExecutionContext.cpp:3672`). 그러면 `bPendingInteractResolve`·`NotifyInteractionPending` 을 통째로 지울 수 있고, 태그 없는 중간 상태를 거쳐 같은 태그로 돌아오는 경우도 재진입으로 잡힌다. 최소 수정만 하려면 `BroadcastInteractionDelegate` 가 `BroadcastDelegate` 의 반환값을 돌려주고 `false` 면 플래그를 세우지 않게 하되, 그 방법으론 위 ①③은 막지 못한다.
- **확신도**: 높음

### 2. 🟡 상호작용 대기 등록이 통보자의 월드로 대상을 해석해 PIE 다중 월드에서 서로의 대기를 완료시킨다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:55`
- **범주**: 설계/구조 (월드 스코프)
- **문제**: 대기 목록 `InteractionWaits` 는 프로세스 전역이고(`:11-25`), 판정은 `Wait.Target.SyncFind(Target) == Target` 으로 **통보자 액터를 해석 컨텍스트로** 쓴다. UE 5.8 `FActorLocatorFragment::Resolve` 는 컨텍스트의 레벨과 PIE 인스턴스 ID 로 경로를 픽스업하므로(`Engine/Private/UniversalObjectLocators/ActorLocatorFragment.cpp:110-142`), 클라 월드에서 등록된 대기도 서버 월드의 액터로 해석되어 `== Target` 이 성립한다. 즉 PIE 로 서버+클라를 한 프로세스에 띄우면 서버 쪽 상호작용 한 번이 클라 월드의 같은 대상 대기까지 함께 완료시킨다(권위 전용이라는 계약은 문서에만 있고 `EnterState` 에 권위 게이트가 없다). 형제 태스크인 `WxStateTreeTask_WaitSpawnersKilled.cpp:64-72` 는 반대로 **대기 자신의 오너**를 컨텍스트로 넘겨 자기 월드에서 해석하므로 같은 문제가 없다 — 두 파일이 갈려 있는 것이 이 지점이다.
- **제안**: `Wait.Context.GetOwner()` 를 컨텍스트로 `SyncFind` 한 결과를 `Target` 과 비교한다(형제 태스크와 동일 형태). 등록 시 오너의 월드를 함께 담아 통보자의 월드와 다르면 건너뛰는 방식도 같다.
- **확신도**: 중간

### 3. 🟢 추종 대상 상태를 선택할 수 없을 때 매 틱 전이 요청이 무한 반복된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:150-178`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:213-233`
- **범주**: 성능/안전
- **문제**: `FollowStateTag` 는 태그가 에셋에 아예 없는 경우만 포기하고(`:152-158`), 태그는 있으나 그 상태(또는 조상)의 진입 조건이 로컬에서 실패해 선택되지 않는 경우는 감지하지 않는다. `RequestState` 는 다음 틱을 예약하므로 「요청 → 선택 실패 → 어긋남 → 재요청」이 매 프레임 돌며 트리가 영영 잠들지 못하고, 로그가 Verbose 라 조립 실수도 드러나지 않는다. 권위 측 복원 수렴도 같은 경로다.
- **제안**: 같은 목표로 연속 실패한 횟수를 세어 한계에서 Warning 한 번 남기고 추종을 접거나 요청 간격을 늘린다. 진입 조건으로 추종을 막는 상태를 에셋 규약으로 금지하는 편이 근본 해법이다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 로케이터 표시명 3벌·컴파일 검증 2벌이 그대로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:113-135`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106-128`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43-65`; `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp:58-78`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:127-147`
- **범주**: 중복/복잡도
- **문제**: `GetTargetDisplayName` 두 벌은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 과 빈 로케이터 문구("unset"/"none")만 다른 동일 코드다. 두 스포너 태스크의 `Compile()` 은 주석·오류 문구까지 같은 검증 루프다. 로케이터 해석 규칙이 바뀌면 다섯 군데를 함께 고쳐야 한다.
- **제안**: 에디터 전용 표시명은 `UWxSpawnerLibrary` 의 것을 이름만 일반화해(`GetActorLocatorDisplayName`) 한 곳에 두고, `Compile()` 검증은 `(로케이터 배열, 기대 클래스, CompileContext)` 를 받는 정적 헬퍼 하나로 합친다.
- **확신도**: 높음

### 5. 🟢 `GetPrompts()` 가 소멸한 후보를 건너뛰어 `GetSelectedIndex()` 와 인덱스가 어긋날 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:73-87`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:89-101`
- **범주**: 버그/정확성
- **문제**: `InRangeActors` 의 약참조가 스캔 사이(최대 `ScanInterval`)에 무효화되면 `GetPrompts` 는 그 자리를 비우지 않고 건너뛰지만 `GetSelectedIndex`/`GetSelectedActor` 는 원래 인덱스를 그대로 쓴다. 주석(`:81`)은 「인덱스 정합을 위해 빈 텍스트로 자리를 채운다」고 하지만 그 채움은 인터페이스 미구현 경로에만 걸리고 무효 약참조에는 적용되지 않는다. 뷰모델이 바인딩 시점에 두 값을 시드로 읽는(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43-44`) 그 순간에 걸리면 선택 표시가 한 칸 밀린다. `UpdateInRange` 경유의 push 는 앞서 무효 항목을 걷어내므로 영향받지 않는다.
- **제안**: 무효 항목도 `FText::GetEmpty()` 로 자리를 채우거나, 두 게터가 읽기 전에 무효 항목을 먼저 걷어낸다(`UpdateInRange` 의 제거 루프 재사용).
- **확신도**: 중간

### 6. 🟢 '이벤트 보내기' 의 실패 반환이 무효라 내장 장치 지목 실패가 로그로만 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SendEvent.cpp:74`
- **범주**: 버그/정확성
- **문제**: `ChildDevice` 지목을 해석하지 못하면 Error 로그 뒤 `EStateTreeRunStatus::Failed` 를 반환하지만, 이 태스크는 생성자에서 `bConsideredForCompletion = false` 로 못박혀 있다(`:6-8`). UE 5.8 은 EnterState 반환을 `IsConsideredForCompletion` 인 태스크에 대해서만 상태 실패로 승격하고(`StateTreeExecutionContext.cpp:3873-3880`), `HasAnyFailed()` 도 CompletionMask 로 걸러 뒤 태스크의 틱까지 막지 않는다(`StateTreeTasksStatus.h:159-162`). 즉 이 `Failed` 는 아무 효과가 없어, 코드를 읽는 사람은 실패 전이가 걸린다고 오해하고 실제로는 장치가 조용히 아무 것도 보내지 않은 채 그 상태를 계속 진행한다.
- **제안**: 반환을 `Succeeded` 로 바꿔 「로그가 유일한 신호」임을 코드에도 드러내거나, 정말 상태를 실패시켜야 하면 이 태스크만 완료 판정에 참여시킨다(후자는 즉발 태스크 규약과 충돌하므로 전자 권장).
- **확신도**: 높음

### 7. 🟢 Build.cs 에 쓰이지 않는 모듈 의존이 남아 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:29`
- **범주**: 중복/복잡도
- **문제**: `AIModule` 은 모듈 전체에서 참조가 없고(주석 한 줄에 AIController 가 언급될 뿐), `GameplayTasks` 는 `GameplayAbilities` 가 이미 Public 으로 끌고 오는 것을 다시 적은 것이다. 도메인 경계를 읽을 때 「WxWorld 가 AI 를 안다」는 잘못된 인상을 준다.
- **제안**: 두 줄을 지우고 빌드가 통과하는지 확인한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_EnableInteraction.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxComponentName.cpp` — 그리고 교차 확인용 UE 5.8 엔진 소스(`StateTreeExecutionContext.cpp`, `StateTreeAsyncExecutionContext.cpp`, `StateTreeExecutionTypes.cpp`, `StateTreeTasksStatus.h`, `StateTreeComponent.cpp`, `ChildActorComponent.cpp`, `ActorLocatorFragment.cpp`), 호출부 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`·`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`.
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/` 의 연출·이동 태스크 전부(`WxStateTreeTask_ComponentMove.cpp`, `WxStateTreeTask_SplineMove.cpp`, `WxStateTreeTask_PlayAnimation.cpp`, `WxStateTreeTask_PlayLevelSequence.cpp`, `WxStateTreeTask_PlaySound.cpp`, `WxStateTreeTask_SpawnNiagara.cpp`, `WxStateTreeTask_PlayInteractorMontage.cpp`, `WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `WxStateTreeTask_EnablePlayerInput.cpp`, `WxStateTreeTask_TriggerSpawners.cpp`, `WxStateTreeTask_RespawnSpawners.cpp`)와 `Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Private/Spawnable/WxSpawnable.cpp`, `Private/System/WxSpawnerLibrary.cpp`, `Private/System/WxWorldDeveloperSettings.cpp`, `Private/WxWorldModule.cpp` 및 이들의 헤더(인스턴스 데이터의 UPROPERTY GC 안전성은 전수 확인 — 런타임 캐시 포인터에 누락 없음), `Plugins/WxWorld/WxWorld.uplugin`.
- **미검토 / 한계**: BP·ST 에셋 내부(각 장치의 상태 구성, 어느 상태가 상호작용을 끄는지, 전이 조건)는 범위 밖이라 발견 1의 발현 빈도는 에셋 조립에 따라 달라진다. 멀티플레이·PIE 동작은 정적 분석과 엔진 소스 확인으로만 검증했고 실측하지 않았다(발견 2 포함). `AWxSpawner`·`AWxDevice` 의 `SaveId` 가 런타임 스폰 인스턴스에서 빈 GUID 로 남는 문제는 소비 측(WxSave)의 키 정책에 달려 있어 이 리뷰에서 판정하지 않았다. 에디터 전용 프리뷰(`AWxSpawner::UpdateEditorPreviewFromSpawnableClass`)는 로직만 읽었고 실제 에디터 동작은 확인하지 않았다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 50파일 — `/module-review`로 갱신*
