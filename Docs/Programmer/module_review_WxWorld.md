# WxWorld — 코드 리뷰

> 전반적으로 건강한 모듈이다 — 서버 권위·복제 추종·세이브 복원 모델이 doc-comment 에 명확히 적혀 있고 구현이 그 설명과 일치하며, CLAUDE.md 규칙 위반(저작권 줄·Prefix·BlueprintCallable·인라인·플러그인 경계·`Super::` 누락)은 50파일 전부에서 발견되지 않았다. 이번 리뷰는 최근 재편된 Device 계열(`AWxDevice`·`UWxDeviceStateTreeComponent`·`AWxTriggerDevice`)과 상호작용 스캐너·상호작용 ST 태스크·스포너를 cpp 까지 깊게 보고, 나머지 연출 태스크는 수명주기·권위 게이트 위주로 훑었으며, 복제·StateTree 동작은 UE 5.8 엔진 소스로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 「전이 없음」을 「같은 상태 재진입」으로 오판해 클라만 현재 상태를 재진입한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:84-86`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:149`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:142-146`
- **범주**: 버그/정확성 (권위 모델)
- **문제**: 권위 상호작용은 `NotifyInteractionPending()` 으로 플래그를 세운 뒤 델리게이트를 발행하고, 다음 `PublishAuthorityState` 는 「플래그가 서 있는데 StateTag 가 그대로」를 곧 자기 전이(재진입)로 간주해 `Multicast_ReenterState` 를 보낸다. 그러나 발행이 트리에 닿지 않아 전이가 아예 없었던 경우도 같은 관측 결과를 낸다. UE 5.8 `TStateTreeStrongExecutionContext::BroadcastDelegate` 는 (a) 바인딩을 남긴 상태가 이미 비활성이면 발행 없이 `false` 를 답하고, (b) 빈 Dispatcher 면 아무것도 하지 않고 `true` 를 답하는데, `BroadcastInteractionDelegate` 는 이 반환값을 버린다(`WxDevice.cpp:149`). 구체 시나리오: ① 바인딩은 상태를 떠나도 남으므로(`Public/Interaction/WxStateTreeTask_EnableInteraction.h:40`) 이전 상태가 켜 둔 상호작용이 다음 상태(예: 문의 Closing/Opening)에서 눌리면 발행은 실패하고 플래그만 남아, 그 상태가 Tag 를 가지면 즉시, 없으면 이후 어떤 틱에서든(발동 장치 이벤트·복원) 헛 재진입 멀티캐스트가 나간다. ② 남의 트리가 `SetInteractionEnabled(true)` 로 켠 장치(`WxDevice.cpp:55-58`, Dispatcher 없음)를 누른 경우도 동일. ③ 전이가 듣고는 있으나 전이 조건(열쇠 보유 등)이 서버에서 실패한 경우도 동일. 결과는 서버는 가만히 있는데 클라만 `RequestState(StateTag)` 로 현재 상태를 재선택해 `bShouldStateChangeOnReselect` 기본값(true)인 PlayAnimation·PlaySound·SpawnNiagara·PlayInteractorMontage·MoveInteractorToTarget 이 다시 실행된다 — 문 애니 재시작, 사운드 재생, 당사자 몽타주·이동 재연출이 클라에서만 일어나는 눈에 보이는 디싱크다. `NotifyDeviceInteracted` 의 주석(`WxDevice.cpp:124-125`)이 이벤트 경로에서 피하려 한 바로 그 현상이 자기 상호작용 경로에는 열려 있다.
- **제안**: 추론 대신 트리의 사실을 읽는다. `UWxDeviceStateTreeComponent::TickComponent` 에서 `Super::TickComponent` 전에 `FStateTreeReadOnlyExecutionContext::GetStateChangeCount()` 를 읽어 두고, 틱 뒤 `ActiveTag == StateTag` 이면서 카운트가 올랐을 때만 `Multicast_ReenterState` 를 보낸다(엔진은 같은 상태 재선택을 포함한 모든 상태 선택에서 이 카운트를 올린다). 그러면 `bPendingInteractResolve`·`NotifyInteractionPending` 은 통째로 지울 수 있고, 태그 없는 중간 상태를 거쳐 같은 태그로 돌아오는 경우도 그대로 재진입으로 잡힌다. 최소 수정만 하려면 `BroadcastInteractionDelegate` 가 `BroadcastDelegate` 의 반환값을 돌려주고 `OnInteracted` 가 `false` 면 플래그를 세우지 않게 하되, 이 방법은 위 ③(조건 실패)은 막지 못한다.
- **확신도**: 높음

### 2. 🟡 ChildActor 로 심긴 발동 장치의 자기 배선이 리모트 클라에서 비어 프롬프트 게이트가 갈린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxTriggerDevice.cpp:97-103`
- **범주**: 설계/구조 (복제)
- **문제**: `BeginPlay` 가 `GetParentComponent()` 로 부모 장치를 찾아 `LinkedDevices` 에 넣는데, 주석(`:96`)대로 `ParentComponent` 는 복제되지 않고 UE 5.8 `UChildActorComponent::OnRep_ChildActor` 도 파괴 델리게이트만 등록할 뿐 이를 세우지 않아 리모트 클라의 `LinkedDevices` 는 빈다. 그러면 `IsInteractionEnabled` 의 `StateTagRequirements` 판정(`:41-48`)이 클라에서 무조건 통과해 잠긴 레버에 프롬프트·하이라이트가 뜨고, 눌러도 서버가 조용히 거부한다(사용자에겐 「안 눌리는 레버」). 「싱글/리슨 호스트 전제」로 수용했다고 적혀 있으나, 모듈의 나머지 전부가 복제를 전제로 설계된 것과 어긋나는 유일한 구멍이고 고치는 비용이 작다.
- **제안**: 부모 식별을 `GetParentComponent()` 대신 `GetOwner()` 로 한다 — `UChildActorComponent::CreateChildActor` 는 자식 액터의 `Owner` 를 자기 오너로 세우고(`Params.Owner = MyOwner`), `AActor::Owner` 는 복제되며 복제 스폰 액터의 `BeginPlay` 는 초기 프로퍼티 수신 뒤에 돌므로 클라에서도 같은 배선이 선다. 배치형(독립) 레버는 Owner 가 없어 영향이 없다. 같은 김에 `LinkedDevices` 가 이미 `TArray<TObjectPtr<AWxDevice>>` 인데 `const AActor*` 로 받아 `Cast<AWxDevice>` 하는 두 루프(`:41-44`, `:77-79`)의 이중 변환도 걷어낸다.
- **확신도**: 중간

### 3. 🟢 추종 대상 상태를 선택할 수 없을 때 매 틱 전이 요청이 무한 반복된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:171-180`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:216-236`
- **범주**: 성능/안전
- **문제**: `FollowStateTag` 는 태그가 에셋에 없는 경우만 포기하고(`:155-160`), 태그는 있으나 그 상태(또는 조상)의 진입 조건이 로컬에서 실패해 선택되지 않는 경우는 감지하지 않는다. `RequestTransition` 은 다음 틱을 예약하므로 「요청 → 선택 실패 → 어긋남 → 재요청」이 매 프레임 돌며 트리가 영영 잠들지 못하고, 로그는 Verbose 라 조립 실수가 드러나지 않는다. 권위 측 복원 추종도 같은 경로다.
- **제안**: `RequestState` 직후 틱에서 여전히 같은 목표로 어긋나 있으면 횟수를 세어 한계(예: 몇 틱)에서 Warning 한 번 남기고 추종을 접거나 요청 간격을 늘린다. 진입 조건으로 클라를 막는 상태는 에셋 규약으로 금지하는 편이 근본 해법이다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 로케이터 표시명·컴파일 검증 헬퍼가 세 벌·두 벌로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:106-128`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106-128`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43-65`; `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp:58-78`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:127-147`
- **범주**: 중복/복잡도
- **문제**: `GetTargetDisplayName` 두 벌은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 과 빈 로케이터 문구("unset"/"none")만 다른 동일 코드이고, 두 스포너 태스크의 `Compile()` 은 오류 문구까지 같은 검증 루프다. 로케이터 해석 규칙이 바뀌면 다섯 군데를 같이 고쳐야 한다.
- **제안**: 에디터 전용 표시명은 `UWxSpawnerLibrary` 의 것을 쓰거나 이름을 일반화해(`GetActorLocatorDisplayName`) 한 곳에 두고, `Compile()` 검증은 `(로케이터 배열, 기대 클래스, CompileContext)` 를 받는 정적 헬퍼 하나로 합친다.
- **확신도**: 높음

### 5. 🟢 `GetPrompts()` 가 소멸한 후보를 건너뛰어 `GetSelectedIndex()` 와 인덱스가 어긋날 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:77-86`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:95-102`
- **범주**: 버그/정확성
- **문제**: `InRangeActors` 의 약참조가 스캔 사이(최대 `ScanInterval`)에 무효화되면 `GetPrompts` 는 그 자리를 비우지 않고 건너뛰지만 `GetSelectedIndex`/`GetSelectedActor` 는 원래 인덱스를 그대로 쓴다. 뷰모델이 바인딩 시점에 두 값을 시드로 읽는(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43-44`) 그 순간에 걸리면 선택 표시가 한 칸 밀린다. 주석(`:82`)은 「인덱스 정합을 위해 빈 텍스트로 자리를 채운다」고 하지만 무효 약참조 경로엔 적용되지 않는다.
- **제안**: 무효 항목도 `FText::GetEmpty()` 로 자리를 채우거나, `GetPrompts`/`GetSelectedIndex` 호출 전에 무효 항목을 먼저 걷어낸다(`UpdateInRange` 의 제거 루프 재사용).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxTriggerDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxTriggerDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_EnableInteraction.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_MoveInteractorToTarget.cpp`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin` — 그리고 교차 확인용 UE 5.8 엔진 소스(`StateTreeComponent.cpp`, `StateTreeExecutionContext.cpp`, `StateTreeAsyncExecutionContext.cpp`, `NetDriver.cpp`, `ChildActorComponent.cpp`), 호출부 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`.
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ComponentSplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` 및 이들의 헤더.
- **미검토 / 한계**: BP·ST 에셋 내부(BP_Door 의 상태 구성, 전이 조건, 어느 상태가 상호작용을 끄는지)는 범위 밖이라 발견 1의 발현 빈도는 에셋 조립에 따라 달라진다. 멀티플레이 동작은 정적 분석과 엔진 소스 확인으로만 검증했고 실측하지 않았다. `UWxInteractionScannerComponent::OnAnyScannerReady` 정적 델리게이트의 구독자 해제는 구독 모듈(WxGame/WxUI) 쪽 책임이라 보지 않았다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 50파일 — `/module-review`로 갱신*
