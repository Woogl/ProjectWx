# WxWorld — 코드 리뷰

> 권위/추종 분리, 초기 진입 판별, ExitState 짝맞춤 같은 어려운 지점을 대부분 의식적으로 처리해 둔 건강한 모듈이다. 다만 상호작용 → 재진입 판정 경로에 한 갈래가 빠져 있고, 엔진 내부 함수를 복제한 자리와 태스크 간 중복이 남아 있다. 이번 리뷰는 기믹 컴포넌트·스캐너·레버·스포너의 cpp 전체와 ST 태스크 16종의 진입/틱/이탈 경로를 읽었고, 판정이 갈리는 지점은 UE 5.8 엔진 소스(StateTreeComponent/StateTreeExecutionContext/Level)로 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 5 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 발행자 없는 상호작용이 클라 전용 재진입을 유발해 서버와 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:143`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:394`
- **범주**: 버그/정확성
- **문제**: `SetInteractionEnabled(true)`(남의 트리가 '상호작용 켜기' Target 갈래로 켜는 경로, `WxGimmickStateTreeComponent.cpp:113`)는 빈 `FWxGimmickInteractionBinding` 을 심는다 — Dispatcher 도 Context 도 무효다. 그 상태에서 플레이어가 이 기믹을 직접 누르면 `OnInteracted` 가 `bInteractionEnabled` 게이트를 통과해 `bPendingInteractResolve = true` 를 세우지만, 이어지는 `BroadcastDelegate` 는 무효 Dispatcher 라 엔진에서 조용히 노옵으로 끝난다(`StateTreeAsyncExecutionContext.cpp:105` 의 `if (!Dispatcher.IsValid()) return true;`). 그 뒤 트리가 어떤 이유로든 한 번 틱하면 `PublishAuthorityState` 가 `ActiveTag == StateTag` + 대기 플래그를 「상태가 안 바뀐 재진입」으로 오판해 `Multicast_ReenterState` 를 쏜다. 결과는 서버는 아무 일도 하지 않았는데 **클라만** `EnterReplicatedState()` 로 현재 상태를 Critical 전이로 재선택하는 divergence다 — 사운드/나이아가라/애니 재생이 클라에서만 다시 돌고, 그 상태에 `상호작용자 이동`이 걸려 있으면 `Transition.SourceStateID` 가 유효한 라이브 진입이라 클라에서만 캐릭터가 목표 지점으로 끌려간다. `NotifyDeviceInteracted` 는 정확히 이 실패를 피하려고 플래그를 걸지 않는데(`WxGimmickStateTreeComponent.cpp:197-198` 주석), 같은 구멍이 직접 상호작용 쪽에 남아 있다. 헤더(`WxGimmickStateTreeComponent.h:103`)가 「켜도 눌림이 트리에 닿지 않는다」로 인정한 지원되는 구성이라 도달 가능하다.
- **제안**: `OnInteracted` 에서 `bPendingInteractResolve = true` 를 `InteractionBinding.Dispatcher.IsValid()` (또는 `InteractionBinding.Context` 유효성) 조건으로 감싼다. 트리에 닿지 않을 눌림은 재진입 판정 대상이 아니다.
- **확신도**: 중간

### 2. 🟡 '플레이어 입력 끄기'가 상호작용 당사자가 아니라 이 머신의 첫 로컬 플레이어를 막는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp:34`
- **범주**: 설계/구조
- **문제**: `GEngine->GetFirstLocalPlayerController(...)` 로 대상을 고른다. 기믹 ST 는 모든 피어에서 각자 돌므로, 멀티플레이에서 A 가 레버를 당겨 컷신 상태로 들어가면 그 기믹이 관련성 안에 있는 **B 의 입력까지** 꺼졌다 켜진다. 스플릿스크린이면 반대로 2P 이상은 토글에서 통째로 빠진다. 같은 모듈의 `WxStateTreeTask_MoveInteractorToTarget.cpp:44` 는 `Character->IsLocallyControlled()` 로 정확히 이 문제를 막고 있어 두 태스크의 규약이 어긋나 있다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_EnablePlayerInput.h:42-43`)가 한계로 이미 적어 두었고 해법(`GetInteractingCharacter` 를 읽는다)까지 남겨 두었으므로, 남은 것은 실행뿐이다.
- **제안**: 오너 기믹의 `GetInteractingCharacter()` 를 읽어 `IsLocallyControlled()` 인 피어에서만 그 캐릭터의 컨트롤러를 토글한다. `DisabledPawn`/`DisabledController` 기록·해제 구조는 그대로 쓸 수 있다.
- **확신도**: 높음

### 3. 🟡 `StartTreeAtSavedState` 가 엔진 `StartTree` 를 복제하면서 재진입 가드를 잃었다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:306`
- **범주**: 성능/안전
- **문제**: 엔진 `UStateTreeComponent::StartTree()` 는 본문 앞뒤로 `if (CurrentlyRunningExecContext) { Error; return; }` + `TGuardValue<...> ReentrantExecutionContextGuard(...)` 를 두는데, 이 재구현에는 둘 다 없다. `CurrentlyRunningExecContext` 는 엔진 헤더에서 `private`(`StateTreeComponent.h:196`)이라 파생 클래스가 재현할 수단도 없다. 두 가지가 깨진다 — (a) 재진입 Start 가 막히지 않는다(`OnSaveRestored`→`RestartLogic` 이 트리 틱 안에서 불릴 수 있는 경로다), (b) Start 도중 `StopLogic` 이 들어오면 엔진이 「기존 실행 컨텍스트를 써서 프레임 끝으로 미루는」 갈래를 타지 못하고 새 컨텍스트를 만든다. 저장 상태 시작 파라미터만 필요한 것이므로 복제 범위가 필요 이상으로 넓다. 헤더 주석이 「엔진 업그레이드 시 확인 지점」으로 표시해 둔 자리이기도 하다.
- **제안**: 재구현을 유지한다면 최소한 컴포넌트 자체에 재진입 플래그(`bIsStartingTree` 등)를 두고 같은 조기 반환을 재현한다. 장기적으로는 엔진에 시작 상태 오버라이드 훅을 요청하거나, `SelectStateOverrideArgs` 만 실어 보내는 얇은 경로로 좁힌다.
- **확신도**: 중간

### 4. 🟡 완료 통보 버스가 ST 태스크 struct 의 static 전역 레지스트리다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:12`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:11`
- **범주**: 설계/구조
- **문제**: 대기 목록(`SpawnersKilledWaits`, `InteractionWaits`)과 핸들 카운터가 프로세스 전역 static 이고, 통보 주체가 그 태스크 타입을 직접 알아야 한다 — `AWxSpawner::MarkKilled` 가 태스크 헤더를 include 해 `FWxStateTreeTask_WaitSpawnersKilled::NotifySpawnerKilled()` 를 부르고(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:94`), 어빌리티 쪽도 마찬가지다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:94`). 도메인 액터·어빌리티가 특정 ST 노드 구현에 묶여, 같은 사건을 듣고 싶은 다른 소비자가 생기면 같은 패턴을 복제하게 된다. 수명 측면도 약하다 — 엔트리는 `ExitState` 또는 다음 통보 때의 기회주의적 청소로만 회수되므로, 통보가 더 이상 오지 않는 조합(마지막 스포너가 죽은 뒤 트리가 사라짐, PIE 종료)에서는 프로세스 수명 동안 남는다. 월드가 다른 엔트리는 각자 오너로 해석하므로 오작동은 없지만, 세션 경계에서 비워 주는 자리가 없다.
- **제안**: 두 통보를 월드 서브시스템(또는 `AWxSpawner` 의 델리게이트 + 스포너를 아는 중개자) 한 곳으로 모으고, 태스크는 그 델리게이트를 구독하는 쪽으로 바꾼다. 최소 조치로는 월드 정리 훅에서 배열을 비우고, 통보 주체가 태스크 헤더 대신 그 중개자만 알게 한다.
- **확신도**: 중간

### 5. 🟡 로케이터 표시명 헬퍼가 세 벌 중복돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:107`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43`
- **범주**: 중복/복잡도
- **문제**: 「해석되면 액터 라벨, 미해석이면 `FActorLocatorFragment` 서브패스의 마지막 토큰, 빈 로케이터는 unset/none」 로직이 문장 단위로 동일하게 세 번 적혀 있다. 셋 중 하나(`UWxSpawnerLibrary::GetSpawnerLocatorDisplayName`)는 이미 모듈 공개 API 이고 같은 폴더의 다른 태스크들이 그것을 쓰고 있다. 표시 규칙이 바뀌면 세 군데를 같이 고쳐야 한다.
- **제안**: 두 상호작용 태스크의 `GetTargetDisplayName` 을 지우고 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 을 부른다(빈 값 표기가 `unset`/`none` 으로 갈리는 것은 호출부에서 처리). 이름이 스포너에 묶여 있는 것이 걸리면 라이브러리 함수명을 `GetLocatorDisplayName` 으로 중립화한다.
- **확신도**: 높음

### 6. 🟡 스포너가 셀 언로드에서도 스폰 인스턴스를 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:134`
- **범주**: 설계/구조
- **문제**: `EndPlay` 가 사유를 가리지 않고 `SpawnedActor` 를 `Destroy()` 한다. World Partition 에서 스포너가 있는 셀만 언로드되는 상황(`EEndPlayReason::RemovedFromWorld`)에 스폰된 적이 플레이어를 쫓아 다른(아직 로드된) 셀로 넘어와 있으면 전투 중에 그대로 사라진다. `bIsKilled` 는 그대로 false 이므로 셀이 다시 로드되면 `BeginPlay` 가 만전 상태로 새로 스폰한다. 셀 경계에서 어그로가 끊기는 것을 의도한 정책일 수 있으나, 코드에는 그 판단의 흔적이 없다.
- **제안**: 의도라면 사유별 분기 없이 그 정책을 주석으로 못 박고, 아니라면 `EndPlayReason == RemovedFromWorld` 일 때 파괴 대신 약참조만 놓아 스폰 인스턴스의 수명을 자기 셀에 맡긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 7. 🟢 `GetPrompts()` 가 죽은 약참조를 건너뛰어 선택 인덱스와 어긋날 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:77`
- **범주**: 버그/정확성
- **문제**: 바로 위 주석(`:82`)은 「인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다」라고 선언하는데, 자리를 채우는 것은 `IWxInteractable` 미구현 갈래뿐이고 `Weak.Get()` 이 null 인 항목은 통째로 건너뛴다. 그러면 반환 배열이 `InRangeActors` 보다 짧아져 뷰모델에 함께 전달되는 `SelectedIndex`(`:128`)가 엉뚱한 문구를 가리킨다. 스캔 경로는 `UpdateInRange` 가 먼저 죽은 항목을 걷어내므로(`:207-216`) 안전하고, 뷰모델이 초기 시드로 바깥에서 직접 부르는 경로에서만 노출된다.
- **제안**: 바깥 `if` 를 없애고 죽은 항목도 `FText::GetEmpty()` 로 자리를 채우거나(주석이 말하는 동작), 반대로 주석을 실제 동작에 맞춘 뒤 시드 경로에서 프루닝을 먼저 돌린다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxLeverDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ComponentSplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, 모든 `Public/` 헤더, 대조용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` 와 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`
- **규칙 점검 결과**: 전 파일 첫 줄 저작권 표기 정상, `FORCEINLINE`·인라인 정의 없음(헤더의 `GetInstanceDataType()` 은 각 헤더가 명시한 규칙 6 예외), `BlueprintCallable` 은 `UWxSpawnerLibrary` 한 곳뿐(BP Function Library 로 허용 범위), 람다는 스캐너의 정렬 술어 1개(필요), Wx 의존은 `WxCore` 뿐 — 모듈 규칙 위반 없음.
- **미검토 / 한계**: ST 에셋·BP 기믹의 실제 전이 저작(상태 Tag 유일성, 어떤 상태가 Target 갈래로 열리는지)은 C++ 밖이라 확인하지 못했다 — 1번 발견의 실제 발생 빈도는 그 저작에 달려 있다. `#if WITH_EDITOR` 경로(스포너 프리뷰 자식 액터, `PreSave` 의 SaveId 확정, `Compile` 검증)는 코드만 읽었고 에디터에서 재현하지는 않았다. 저장/복원 왕복(`IWxSavable` 소비 측 WxSave 오케스트레이션)과 스캐너를 주입하는 Experience 설정도 이번 범위 밖이다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 48파일 — `/module-review`로 갱신*
