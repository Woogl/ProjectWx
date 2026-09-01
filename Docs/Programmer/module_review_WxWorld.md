# WxWorld — 코드 리뷰

> 서버 권위 StateTree 수렴, 통보 기반 대기, 스포너 수명이라는 세 축의 골격이 견고하고 코딩·모듈 규칙 위반은 한 건도 없다. 남은 문제는 경계 조건(링크 서브트리, 대상 소실, WP 셀 로케이터 해석)과 세이브 제거 뒤 남은 잔재에 몰려 있다. 51개 소스 전부를 규칙 관점에서 훑고 장치 상태머신·상호작용 스캐너·스포너·대기 등록부와 13개 StateTree 태스크 구현을 깊게 봤으며, 계약 상대인 `WxCore`의 `IWxInteractable`·`WxGame`의 `UWxAbility_Interact`, 그리고 엔진 `UStateTreeComponent` 구현까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 링크 서브트리의 상태 Tag 는 발행되지만 추종할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:206`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:236`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:258`
- **범주**: 설계/구조
- **문제**: `GetActiveStateTag()` 는 활성 프레임 전체를 역순으로 훑어 링크된 서브트리 에셋의 Tag 까지 `StateTagName` 으로 발행한다. 반면 `RequestState()`·`HasState()` 는 루트 `StateTreeRef.GetStateTree()` 에서만 핸들을 찾는다. 서브트리 상태가 발행되면 클라 추종은 `RequestState` 가 무효 핸들로 조용히 노옵해 영영 어긋나고(매 틱 Verbose 로그만 남는다), `OnRep_StateTagName` 은 "권위 상태를 로컬 에셋에서 찾지 못했다" 경고를 서버·클라 에셋 불일치로 오진하게 만든다.
- **제안**: 상태 키 계약을 루트 에셋 Tag 로 좁혀 `GetActiveStateTag()` 의 탐색을 루트 프레임으로 한정하거나, 조회·전이 요청도 활성 프레임의 에셋까지 대칭으로 확장한다.
- **확신도**: 중간

### 2. 🟡 재생 중 상호작용자가 사라지면 몽타주 태스크가 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:45`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상·몽타주가 없으면 `Succeeded` 로 빠지는데, 재생 중 `InteractingCharacter` 가 사망·언포제스·파괴되면 `Failed` 를 반환한다. 헤더 doc-comment(`WxStateTreeTask_PlayInteractorMontage.h:29`)가 "폴링은 대상이 사라진 것까지 종료로 본다"고 계약을 밝히는데 구현이 그 계약과 어긋난다. On Succeeded 전이만 저작한 장치 에셋은 여기서 상태가 갇히고, 실패가 루트까지 전파되면 트리가 멈춘다.
- **제안**: 재생 중 대상 소실도 진입 경로와 같이 `Succeeded` 로 처리하고, 필요하면 Verbose 로그만 남긴다.
- **확신도**: 높음

### 3. 🟡 스포너 로케이터 해석 방식이 두 태스크에서 비대칭이라 WP 셀 대상이 갈린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:42`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:143`
- **범주**: 버그/정확성
- **문제**: 두 태스크의 헤더가 똑같이 "레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다"고 선언하는데 해석 방법이 다르다. `TriggerSpawners` 는 `Locator.SyncFind(Owner)` 로 오너를 컨텍스트 삼고, `WaitSpawnersKilled` 는 월드의 실제 스포너를 순회해 각 후보를 컨텍스트로 맞춰 본다. 후자가 그 모양인 이유는 `WxStateTreeTask_WaitForInteraction.cpp:75` 가 밝힌 그대로다 — "오너인 GameState 는 PersistentLevel 이라 WP 런타임 셀 안의 대상을 해석하지 못한다". 결국 퀘스트 ST 가 WP 셀 안의 스포너를 발동시키려 하면 `TriggerSpawners` 만 "해석된 스포너가 없음" 경고를 남기고 조용히 스폰하지 않는 반면, 같은 스포너를 기다리는 `WaitSpawnersKilled` 는 해석에 성공해 스텝이 영영 끝나지 않는 증상으로 나타난다.
- **제안**: 해석 경로를 `FWxSpawnerLocatorUtils` 의 공용 헬퍼로 모아 두 태스크가 같은 방식(액터 순회 매칭)으로 찾게 한다. 오너 컨텍스트 해석을 유지하려면 최소한 실패 시 순회 폴백을 둔다.
- **확신도**: 중간

### 4. 🟡 처치 대기 폴링이 미해석 로케이터마다 월드 전체 액터를 훑는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:143`
- **범주**: 성능/안전
- **문제**: `ResolveSpawner()` 의 약참조 캐시는 해석에 성공했을 때만 채워지므로, 스트리밍 아웃된 로케이터는 0.25초마다(`SpawnerKilledCheckInterval`) `TActorIterator<AWxSpawner>` 로 레벨 액터 배열 전체를 훑고 후보마다 `Locator.SyncFind` 를 부른다. 이 경로는 예외 상황이 아니라 정상 대기 상태다 — 플레이어가 목표 셀에 도착하기 전까지 퀘스트 스텝 내내, 지정 스포너 수만큼 반복된다.
- **제안**: 실패한 해석은 백오프를 두거나(예: 1~2초), 월드 순회를 태스크당 한 패스로 모아 미해석 로케이터 전부를 한 번에 매칭한다.
- **확신도**: 중간

### 5. 🟢 죽은 약참조를 건너뛰어 프롬프트와 선택 인덱스가 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: `GetPrompts()` 는 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 주석을 달았지만, 그 폴백은 인터페이스 캐스트 실패에만 걸리고 약참조가 무효이면 항목 자체를 넣지 않는다. `UpdateInRange` 안에서 부를 때는 직전에 무효 항목을 걷어내 무해하지만, 뷰모델이 `GetPrompts()`/`GetSelectedIndex()` 를 각각 읽어 초기 시드로 삼는 경로(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43`)에서는 스캔 사이(0.1s)에 앞쪽 대상이 파괴되면 배열이 당겨져 선택 인덱스가 다른 프롬프트를 가리킨다.
- **제안**: 무효 약참조에도 `FText::GetEmpty()` 를 넣어 `InRangeActors` 와 길이·인덱스를 항상 맞춘다.
- **확신도**: 높음

### 6. 🟢 스포너에 제거된 세이브·LSP 를 전제한 주석과 죽은 가드가 남아 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h:16`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:156`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:197`
- **범주**: 중복/복잡도
- **문제**: `EWxSpawnerMode::Auto` 는 "LSP가 레벨 상태를 확정한 뒤 자동으로 스폰한다"고 적혀 있지만 실제 트리거는 `BeginPlay` 다(`WxSave`·LSP 는 09-01 에 전면 제거됐다). 같은 이유로 `SpawnTarget()` 의 "복원된 인스턴스가 붙어 있어 생성 시도를 건너뛴다" 가드는 이제 도달할 수 없다 — 이 함수를 부르는 두 경로 중 `Respawn()` 은 직전에 부착 액터를 모두 Destroy 하고, `BeginPlay` 시점에는 부착된 인스턴스를 만들 주체가 없다. 스폰 타이밍을 다시 손볼 사람이 존재하지 않는 영속 계층을 전제로 읽게 된다.
- **제안**: `Auto`/`Manual` 주석을 BeginPlay 기준으로 정정하고, 부착 인스턴스 가드는 제거하거나 "레벨에 손으로 붙여 둔 액터 보호"라는 현재의 실제 의미로 다시 쓴다.
- **확신도**: 중간

### 7. 🟢 Public 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:28`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h` 를 포함하는데 그 소속인 `GameplayStateTreeModule` 은 `PrivateDependencyModuleNames` 에 있다. 그 헤더는 다시 `UBrainComponent`(AIModule)·`IGameplayTaskOwnerInterface`(GameplayTasks) 를 노출하므로 세 모듈이 모두 Public 표면에 걸려 있다. 지금은 `UWxDeviceStateTreeComponent` 가 `WXWORLD_API` 로 export 되지 않아(`Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에도 그 사실이 기록돼 있다) 밖에서 포함할 일이 없어 드러나지 않을 뿐이며, 언젠가 export 하면 소비 모듈에서 include 경로가 끊긴다.
- **제안**: `GameplayStateTreeModule`·`AIModule`·`GameplayTasks` 를 `PublicDependencyModuleNames` 로 옮긴다(셋 다 상속 계층에 필요하므로 제거는 불가하다).
- **확신도**: 중간

### 8. 🟢 IsAwaited 만 null 대상을 막지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:37`
- **범주**: 버그/정확성
- **문제**: 짝인 `NotifyInteracted()` 는 `Target` null 을 걸러내는데 `IsAwaited()` 는 곧바로 `Target->GetWorld()` 를 부른다. 둘 다 `WXWORLD_API` 로 열린 정적 진입점이고, 술어 `IsWaitingFor()` 도 대상을 무조건 역참조한다. 현재 유일한 호출부(`Source/WxGame/Character/WxNpc.cpp:43`)가 `this` 를 넘겨 실제 사고는 없지만, 두 진입점의 계약이 다르다.
- **제안**: `IsAwaited()` 에도 같은 null 가드를 둔다.
- **확신도**: 중간

### 9. 🟢 스포너의 부착 인스턴스 정리 블록이 두 곳에 복사돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:64`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:124`
- **범주**: 중복/복잡도
- **문제**: `Respawn()` 과 `EndPlay()` 가 "추적 인스턴스 Destroy → 부착 액터 순회 Destroy → 약참조 Reset" 아홉 줄을 그대로 공유한다. 한쪽만 고치면 스트림 아웃 경로나 리스폰 경로 중 하나에서 인스턴스가 남는다.
- **제안**: 한 줄짜리 private 정리 함수로 모은다. 다만 이 프로젝트는 소규모 반복을 용인하는 쪽이므로 급하지 않다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Public/` 전체 헤더, `Private/StateTreeTask/` 의 나머지 태스크(`PlayAnimation`·`PlaySound`·`SpawnNiagara`·`ApplyGameplayEffectToInteractor`·`RespawnSpawners`), `Private/System/` 전체, `Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Private/Device/WxDeviceComponentName.cpp`, `Private/Spawnable/WxSpawnable.cpp`, `Private/WxWorldModule.cpp`
- **교차 확인**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`(`ServerInteract` 가 검증 없는 RPC 여도 서버가 자격·사거리를 다시 판정하고 스캔 반경 150cm 가 양쪽에 같은 값으로 선언돼 있음을 확인), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(프롬프트·선택 인덱스를 따로 시드함), `Source/WxGame/Character/WxNpc.cpp`·`WxEnemyCharacter.cpp`(`IsAwaited`·`MarkKilled`·`OnSpawnedBy` 호출부), 엔진 `UStateTreeComponent`(`ConditionalEnableTick()` 이 다음 프레임 틱을 강제 예약하므로 OnRep 후 수렴이 보장됨을 확인 — 잠든 트리 미수렴 의심은 기각)
- **미검토 / 한계**: StateTree·장치 BP 에셋의 실제 Tag·전이 조립과 WBP 내부는 범위 밖이다. 1·3·4번은 실행 재현 없이 코드 경로로만 검토했으므로 실제 배치(링크 서브트리 사용 여부, 퀘스트 ST 의 스포너 지정이 WP 셀에 있는지)에서 재현 조건이 성립하지 않을 수 있다. 규칙 준수는 51개 소스 전체를 검색해 확인했다 — 첫 줄 저작권 전부 존재, `WxCore` 외 Wx 플러그인 참조 없음, `Wx` prefix 준수, `BlueprintCallable` 은 `UWxSpawnerLibrary` 한 곳뿐(Blueprint Function Library 라 적법), `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 은 전 태스크가 예외 사유 주석을 붙였다), 델리게이트 바인딩 대상은 타이머 콜백 `HandleScanTimer` 하나이며 `Handle` prefix 를 지킨다. 람다는 스캐너의 거리 정렬 술어 하나로 캡처가 필요한 정당한 사용이다. 직전 리뷰의 🟡 "하이라이트 해제가 숨겨진 프리미티브를 건너뛴다" 는 09-01 수정으로 해소됐다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 51파일 — `/module-review`로 갱신*
