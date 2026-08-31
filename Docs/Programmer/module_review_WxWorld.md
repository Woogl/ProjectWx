# WxWorld — 코드 리뷰

> 서버 권위 StateTree 수렴·통보 기반 대기·스포너 수명이라는 세 축의 골격은 견고하고 규칙 위반도 없다. 남은 문제는 대부분 경계 조건(숨겨진 프리미티브, 링크 서브트리, 대상 소실, WP 셀 해석)에 몰려 있다. 51개 C++ 소스 전부의 규칙 준수를 훑고 장치 상태머신·상호작용 스캐너·스포너·대기 등록부·13개 StateTree 태스크 구현을 깊게 봤으며, 계약 상대인 `WxCore`의 `IWxInteractable`, `WxGame`의 `UWxAbility_Interact`, `WxSave`의 레벨 복원 훅까지 교차 확인했다. 직전 리뷰의 🔴(복원된 처치 상태가 통보되지 않아 영구 대기)는 `WaitSpawnersKilled` 가 저주기 Scheduled Tick 재판정으로 바뀌면서 해소됐다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 하이라이트 해제가 숨겨진 프리미티브를 건너뛴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:287`
- **범주**: 버그/정확성
- **문제**: `SetActorHighlighted()` 는 켜기·끄기를 구분하지 않고 `IsVisible()` 이 거짓인 프리미티브를 건너뛴다. 선택된 메시가 장치 연출(문이 열리며 메시 교체)이나 LOD·`SetVisibility` 로 숨겨진 동안 선택이 풀리면 `SetRenderCustomDepth(false)` 가 적용되지 않아, 다시 보일 때 선택되지도 않은 메시가 이전 외곽선을 그대로 유지한다. 직전 리뷰에서 지적된 뒤 그대로 남아 있다.
- **제안**: 가시성 필터는 `bHighlighted` 가 참일 때만 적용하고, 끌 때는 모든 `UPrimitiveComponent` 의 Custom Depth 를 무조건 끈다.
- **확신도**: 높음

### 2. 🟡 링크된 서브트리의 상태 Tag 는 발행되지만 추종·복원할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:221`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:251`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:273`
- **범주**: 설계/구조
- **문제**: `GetActiveStateTag()` 는 활성 프레임 전체를 역순으로 훑어 링크된 서브트리 에셋의 Tag 까지 `StateTagName` 으로 발행한다. 반면 `RequestState()`·`HasState()` 는 루트 `StateTreeRef.GetStateTree()` 에서만 핸들을 찾는다. 서브트리 상태가 발행·저장되면 클라 추종은 `RequestState` 가 무효 핸들로 조용히 노옵해 영영 어긋나고, 권위 복원은 `HasState` 실패로 "복원을 포기한다" 경로를 타 저장된 상태를 잃는다. 직전 리뷰 지적이 유지된 항목이다.
- **제안**: 상태 키 계약을 루트 에셋 Tag 로 좁혀 발행 범위를 제한하거나(권장: `GetActiveStateTag` 를 루트 프레임으로 한정), 조회·전이 요청도 활성 프레임의 에셋까지 대칭적으로 확장한다.
- **확신도**: 중간

### 3. 🟡 재생 중 상호작용자가 사라지면 몽타주 태스크가 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:45`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상·몽타주가 없으면 `Succeeded` 로 빠지지만, 재생 중 `InteractingCharacter` 가 사망·언포제스·파괴되면 `Failed` 를 반환한다. 헤더 doc-comment 는 "폴링은 대상이 사라진 것까지 종료로 본다"고 계약을 밝히므로 구현이 그 계약과 어긋나며, 실패 전이를 저작하지 않은 장치 에셋은 여기서 상태가 갇히거나 트리가 끝난다. 직전 리뷰 지적이 유지된 항목이다.
- **제안**: 재생 중 대상 소실도 진입 경로와 같이 `Succeeded` 로 처리하고, 필요하면 Verbose 로그만 남긴다.
- **확신도**: 높음

### 4. 🟡 스포너 로케이터 해석 방식이 두 태스크에서 비대칭이라 WP 셀 대상이 갈린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:42`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:143`
- **범주**: 버그/정확성
- **문제**: 두 태스크의 헤더는 똑같이 "레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다"고 선언하는데, 해석 방법이 다르다. `TriggerSpawners` 는 `Locator.SyncFind(Owner)` 로 오너를 컨텍스트 삼고, `WaitSpawnersKilled` 는 월드의 실제 스포너를 순회해 각 후보를 컨텍스트로 맞춰 본다. 후자가 그 모양인 이유는 `WxStateTreeTask_WaitForInteraction.cpp:75` 가 밝힌 그대로다 — "오너인 GameState 는 PersistentLevel 이라 WP 런타임 셀 안의 대상을 해석하지 못한다". 즉 퀘스트 ST(오너 GameState)가 WP 셀 안의 스포너를 발동시키려 하면 `TriggerSpawners` 만 조용히 실패해 "해석된 스포너가 없음" 경고만 남기고 스폰이 일어나지 않는다. 같은 스포너를 기다리는 `WaitSpawnersKilled` 는 해석에 성공하므로, 스텝이 끝나지 않는 증상으로 나타난다.
- **제안**: 해석 경로를 `FWxSpawnerLocatorUtils` 에 공용 헬퍼로 모아 두 태스크가 같은 방식(액터 순회 매칭)으로 찾게 한다. 오너 컨텍스트 해석을 유지하려면 최소한 실패 시 순회 폴백을 둔다.
- **확신도**: 중간

### 5. 🟢 죽은 약참조를 건너뛰어 프롬프트와 선택 인덱스가 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: `GetPrompts()` 는 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 주석을 달았지만, 그 폴백은 인터페이스 캐스트 실패에만 걸리고 약참조가 이미 무효이면 항목 자체를 넣지 않는다. `UpdateInRange` 안에서 부를 때는 직전에 무효 항목을 걷어내 문제가 없지만, 뷰모델이 초기 시드로 `GetPrompts()`/`GetSelectedIndex()` 를 각각 읽는 경로에서는 스캔 사이(0.1s)에 앞쪽 대상이 파괴되면 배열이 당겨져 선택 인덱스가 다른 프롬프트를 가리킨다. 직전 리뷰 지적이 유지된 항목이다.
- **제안**: 무효 약참조에도 `FText::GetEmpty()` 를 넣어 `InRangeActors` 와 길이·인덱스를 항상 맞춘다.
- **확신도**: 높음

### 6. 🟢 처치 대기 폴링이 미해석 로케이터마다 월드 전체 액터를 훑는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:143`
- **범주**: 성능/안전
- **문제**: `ResolveSpawner()` 의 캐시는 해석에 성공했을 때만 채워지므로, 스트리밍 아웃 등으로 해석되지 않는 로케이터는 0.25초마다 `TActorIterator<AWxSpawner>` 로 월드 액터 배열 전체를 훑고 후보마다 `Locator.SyncFind` 를 부른다. 대기 태스크가 살아 있는 내내, 지정 스포너 수만큼 이 스캔이 반복된다. 오픈월드에서 로드된 액터가 많고 대상 셀이 언로드된 구간이 길면 무시하기 어려운 비용이 된다.
- **제안**: 실패한 해석도 짧은 시간(예: 1~2초) 백오프하거나, 월드 순회 결과를 태스크 단위로 한 번만 모아 지정 로케이터 전부를 한 패스에 매칭한다.
- **확신도**: 중간

### 7. 🟢 상호작용 예약 플래그가 복원 수렴 구간을 넘어 살아남아 헛재진입을 낸다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:143`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:169`
- **범주**: 버그/정확성
- **문제**: `bPendingInteractResolve` 는 `PublishAuthorityState()` 안에서만 소비되는데, `SyncStateWithTree()` 는 `bFollowRestoredState` 인 동안 `FollowStateTag()` 로만 간다. 복원·초기 상태 수렴이 진행되는 사이(엘리베이터처럼 연출로 이동하는 장치는 이 구간이 수 초다) 플레이어가 상호작용해 발행이 트리에 닿으면 플래그가 서고, 수렴이 끝난 다음 첫 발행 틱에서 "상태가 안 바뀐 재진입" 으로 오판되어 `Multicast_ReenterState` 가 나간다. 클라 전원이 재진입 연출을 한 번 헛재생한다(권위·스탠드얼론은 자체 가드로 무해).
- **제안**: `SyncStateWithTree()` 의 추종 분기에서도 플래그를 소비(또는 리셋)하거나, `NotifyInteractionPending()` 을 `bFollowRestoredState` 동안 무시한다.
- **확신도**: 중간

### 8. 🟢 Public 헤더가 Private 의존 모듈의 헤더를 포함하고, 쓰이지 않는 의존이 남아 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`
- **범주**: 설계/구조
- **문제**: `Components/StateTreeComponent.h` 는 `GameplayStateTreeModule` 소속인데 그 모듈은 `PrivateDependencyModuleNames` 에 있고, 정작 Public 헤더가 그 헤더를 포함한다. 지금은 `UWxDeviceStateTreeComponent` 가 `WXWORLD_API` 로 export 되지 않아(그 사실은 `Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에도 기록돼 있다) 밖에서 포함할 일이 없어 드러나지 않을 뿐이며, 언젠가 export 하는 순간 소비 모듈에서 include 경로가 끊긴다. 같은 배열의 `AIModule`·`GameplayTasks` 는 모듈 전체에서 직접 사용처를 찾지 못했다(전이 의존으로 필요할 수는 있다).
- **제안**: `GameplayStateTreeModule` 을 `PublicDependencyModuleNames` 로 옮긴다. `AIModule`·`GameplayTasks` 는 제거해 보고 빌드가 통과하면 정리한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/Source/WxWorld/Public/` 전체 헤더, `Private/StateTreeTask/` 의 나머지 태스크(`PlayAnimation`·`PlaySound`·`SpawnNiagara`·`ApplyGameplayEffectToInteractor`·`RespawnSpawners`), `Private/System/` 전체, `Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Private/Device/WxDeviceComponentName.cpp`, `Private/WxWorldModule.cpp`
- **교차 확인**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`(`NotBlueprintable` 이라 스캐너의 `Cast<IWxInteractable>` 은 안전), `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`(`ServerInteract` 가 검증 없는 RPC 여도 서버가 자격·사거리를 다시 판정함을 확인), `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp:141`(`OnPostRestoreLevel` 이 저장 유무와 무관하게 레벨마다 호출됨을 확인)
- **미검토 / 한계**: StateTree·장치 BP 에셋의 실제 Tag·전이 조립과 WBP 내부는 범위 밖이다. 네트워크 수렴·월드 파티션 스트리밍·LSP 복원 순서는 실행 재현 없이 코드 경로로만 검토했으므로 4·6·7번은 실제 배치에서 재현 조건이 성립하지 않을 수 있다. 규칙 준수는 51개 소스 전체를 검색해 확인했다 — 첫 줄 저작권(전부 존재, `WxSpawner.cpp` 는 UTF-8 BOM 때문에 단순 grep 에만 걸린다), `WxCore` 외 Wx 플러그인 참조 없음, `Wx` prefix, `BlueprintCallable` 은 `UWxSpawnerLibrary` 한 곳뿐(Blueprint Function Library 로 적법), `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 은 전 태스크가 예외 사유 주석을 붙였다), 델리게이트 바인딩 자체가 없어 `Handle` prefix 대상은 타이머 콜백 `HandleScanTimer` 하나이며 준수한다. 람다는 스캐너의 거리 정렬 술어 하나로, 캡처가 필요한 정당한 사용이다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 51파일 — `/module-review`로 갱신*
