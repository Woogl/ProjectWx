# WxDevice — 코드 리뷰

> **수정 이력(2026-09-06)**: 아래 네 항목은 변경 전 리뷰 기록이다. 실제 전이 관측·복제 스냅샷·초기 수렴·완료 복구를 구현했으며 현재 구조와 검증 범위는 [WxDevice 상태 동기화](WxDevice-state-synchronization.md)에 정리했다.

> `AWxDevice`의 상호작용 표면과 `UWxDeviceStateTreeComponent`의 상태 실행 책임은 분리되어 있으며, 권위 검사와 컴포넌트 이름 해석도 일관된다. 다만 상호작용 발행과 실제 재진입을 구분하는 방식, 태그가 없는 상태 및 트리 완료 처리에서 상태 복제가 끊기는 경로가 있다.
> `WxDevice`는 별도 모듈이 아닌 `WxWorld`의 기능이다. 이번에는 Device와 관련 StateTree 노드 33개 소스 및 외부 호출·에디터 연결 4개 소스를 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 0 |

발견은 특정 StateTree 조립 조건에서 발생하는 P2 수준의 정확성 문제이다. 버그/정확성, 설계/구조, 규칙 위반, 중복/복잡도, 성능/안전의 다섯 차원을 검토했으며, 실질적인 문제가 확인된 항목만 아래에 기록한다.

## 결과

### 1. 🟡 상호작용 전이가 거절되어도 클라이언트에 재진입을 통보한다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:153`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:130`
- **범주**: 버그/정확성
- **문제**: `BroadcastInteractionDelegate()` 성공은 실행 중인 컨텍스트에 델리게이트를 발행했다는 뜻이며, 전이 조건 통과나 상태 재진입을 보장하지 않는다. 그러나 `OnInteracted()`는 성공만으로 `bPendingInteractResolve`를 세우고, 다음 `PublishAuthorityState()`는 활성 태그가 이전과 같으면 무조건 `Multicast_ReenterState()`를 호출한다. 예를 들어 현재 상태의 상호작용 델리게이트 전이에 거짓인 추가 조건을 붙이면 서버는 상태를 유지하지만, 클라이언트는 현재 상태를 다시 선택해 사운드·몽타주 같은 진입 태스크를 실행한다. 지연 전이도 첫 틱에는 태그가 그대로이므로 같은 오판이 발생한다. 설치된 UE 5.8의 `StateTreeAsyncExecutionContext.cpp:111`~`121`은 활성 컨텍스트만으로 발행 성공을 반환하며, `StateTreeExecutionContext.cpp:6057` 및 `6063` 이후에서 조건·지연을 별도로 처리한다.
- **제안**: 발행 예약과 실제 재진입 확인을 분리한다. 활성 상태 인스턴스 ID 변경이나 실제 전이 결과를 관측해 동일 태그 재진입을 판별하고, 조건 실패·지연 대기 중에는 통보하지 않는다. 검증은 거짓 조건, 지연 전이, 정상 자기 전이를 각각 구성해 서버·클라이언트 진입 횟수를 비교한다.
- **확신도**: 높음

### 2. 🟡 태그 없는 기본 대기 상태에서는 유효한 InitialState로 이동하지 못한다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:179`
- **범주**: 버그/정확성
- **문제**: `BeginPlay()`는 유효한 `InitialState`를 찾으면 먼저 `StateTagName`과 `bFollowInitialState`를 설정한 뒤 트리를 기본 경로로 시작한다. 기본 활성 경로 전체에 태그가 없으면 `GetActiveStateTag()`가 무효 태그를 반환하고, `FollowStateTag()`는 목표 태그가 유효해도 여기서 종료한다. 그 기본 상태가 이벤트 대기처럼 스스로 끝나지 않는 상태이면 `RequestState()`에 영원히 도달하지 못해 `InitialState`가 무시되고, 권위 상태 발행도 계속 보류된다. 클라이언트가 같은 태그 없는 기본 상태에서 시작하는 경우에도 서버의 유효 상태를 추종하지 못한다. 이동 중 미태그 구간의 되감기를 피하려는 의도는 타당하지만 최초 수렴도 함께 차단한다.
- **제안**: 최초 목표 수렴과 이미 실행 중인 미태그 중간 구간을 구분한다. 시작 시점에는 활성 태그가 없어도 존재하는 목표 상태로 전이를 요청하거나, 지원하지 않을 조립이라면 컴파일·시작 시 명확히 거절한다. 태그 없는 기본 대기 상태와 태그 있는 형제 목표 상태를 구성하고 `InitialState` 및 레이트조인 경로를 검증한다.
- **확신도**: 높음

### 3. 🟡 연결 장치의 이벤트로 발생한 동일 상태 재진입은 클라이언트에 전달되지 않는다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:87`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:153`
- **범주**: 설계/구조
- **문제**: `FWxStateTreeTask_SendEvent`는 권위에서만 대상 장치의 `NotifyDeviceInteracted()`를 호출한다. 이 함수는 이벤트를 트리에 넣지만 재진입 예약은 하지 않는다. 대상 장치가 해당 이벤트를 받아 같은 태그 상태로 자기 전이하면 서버에서는 진입 태스크가 다시 실행되지만, `StateTagName`은 변하지 않고 `bPendingInteractResolve`도 거짓이라 클라이언트에는 아무 신호도 전달되지 않는다. 결과적으로 버튼에 연결한 대상 장치의 반복 사운드·애니메이션 등은 첫 상태 변경 이후 재발동을 볼 수 없다. 주석에 명시된 미청취 이벤트의 허위 재진입 방지는 필요하지만, 실제로 성립한 이벤트 재진입까지 누락시키고 있다.
- **제안**: 직접 상호작용 여부와 무관하게 실제 동일 태그 재진입을 관측해 통보한다. 단순히 모든 수신 이벤트에서 예약 플래그를 켜면 1번과 같은 허위 재진입이 생기므로 피한다. 연결된 두 장치를 두고 대상의 `OnEvent` 자기 전이를 반복 발동하여 양쪽 피어의 진입 횟수를 검증한다.
- **확신도**: 높음

### 4. 🟡 트리 자동 완료 이후에는 마지막 상태 발행과 클라이언트 추종을 복구하지 못한다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:43`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:179`
- **범주**: 버그/정확성
- **문제**: 상태 관측이 `Super::TickComponent()` 뒤에만 이루어진다. UE 5.8의 자동 완료 경로는 가상 `StopLogic()`를 호출하지 않고 `Context.Tick()` 내부에서 활성 프레임을 비운다. 따라서 한 틱에 마지막 태그 상태 진입과 트리 완료가 발생하면 이 컴포넌트는 마지막 태그를 읽지 못하고 이전 복제 값을 남긴다. 또한 클라이언트의 연출 태스크가 서버보다 먼저 완료되어 트리 전체를 완료시키면, 나중에 서버 태그가 도착해 틱을 깨워도 활성 프레임이 없어서 `FollowStateTag()`가 반환한다. `RequestTransition()` 자체도 활성 프레임 없는 트리를 시작해 주지 않는다. 예를 들어 몽타주가 클라이언트에서 없거나 재생되지 않아 `PlayInteractorMontage`가 즉시 완료되고 완료 전이가 트리를 끝내는 조립에서 발생할 수 있다. `StopLogic()`의 정지 전 동기화는 명시적 정지에만 적용되므로 이 자동 완료 경로를 보호하지 못한다.
- **제안**: 장치 트리가 항상 실행 중이어야 하는 계약이라면 완료 전이를 검증해 거절한다. 완료도 지원한다면 종료 전에 마지막 상태를 확보하고, 완료한 클라이언트 트리를 다시 시작해 권위 상태를 적용하는 복구 경로를 마련한다. 서버의 같은 틱 진입·완료와 클라이언트 선행 완료를 각각 검증한다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Device/` 및 `Private/Device/`의 `WxDevice.h/.cpp`, `WxDeviceStateTreeComponent.h/.cpp`, `WxDeviceComponentName.h/.cpp` 6개. `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/`의 `WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `WxStateTreeTask_ComponentMove.cpp`, `WxStateTreeTask_EnablePlayerInput.cpp`, `WxStateTreeTask_PlayAnimation.cpp`, `WxStateTreeTask_PlayInteractorMontage.cpp`, `WxStateTreeTask_PlayLevelSequence.cpp`, `WxStateTreeTask_PlaySound.cpp`, `WxStateTreeTask_RespawnSpawners.cpp`, `WxStateTreeTask_SendEvent.cpp`, `WxStateTreeTask_SpawnNiagara.cpp`, `WxStateTreeTask_SplineMove.cpp` 11개. `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp` 2개.
- **훑은 파일**: 위 StateTree 태스크 11개의 대응 `Public/StateTreeTask/*.h`, `Public/StateTreeTask/WxStateTreeWaitRegistry.h`, 위 Interaction 태스크 2개의 대응 `Public/Interaction/*.h` 14개. 외부 연결은 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Source/WxEditor/WxDeviceLinkVisualizer.cpp`, `Source/WxEditor/WxStateTreeComponentNameCustomization.cpp` 4개이다.
- **추가 확인**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`. 설치된 UE 5.8 엔진의 `StateTreeAsyncExecutionContext.cpp`, `StateTreeExecutionContext.cpp`, `StateTreeComponent.cpp`에서 델리게이트 반환값·전이 조건·외부 전이 요청·자동 완료 동작을 교차 확인했다. 자동 완료의 근거는 엔진 `StateTreeExecutionContext.cpp:1995`~`2009`, 완료 후 틱 조기 반환은 `1773`, 프레임 없는 외부 요청 거절은 `2235`~`2239`이다.
- **파일 수 기준**: provenance의 37개는 이번 기능 리뷰에서 검토한 저장소 내 `.h`/`.cpp`의 합계이다(Device 6 + StateTreeTask 23 + Interaction 태스크 4 + 외부 연결 4). `WxWorld` 전체 파일 수를 뜻하지 않으며 Build.cs·README·설치 엔진 파일·생성 코드는 제외한다.
- **미검토 / 한계**: BP·StateTree 에셋 내부 그래프와 레벨별 배선은 검사하지 않았다. 따라서 해당 조립이 현재 콘텐츠에 존재하는지는 확인하지 않았다. 소스 기반 정적 검토이며 빌드·PIE·네트워크 재현은 수행하지 않았다. `InteractingCharacter` 속성 복제와 재진입 멀티캐스트의 수신 순서는 별도 검증이 필요해 확정 발견에서 제외했다. 소스 수정 없이 이 리뷰 문서만 생성했다.

---
*문서 기준 커밋 `4abd7dbb` · 리뷰일 2026-09-06 · 소스 37파일 — `/module-review`로 갱신*
