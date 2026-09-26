# WxWorld — 코드 리뷰

> 장치 복제·상호작용의 권위 경계와 참조 정리는 대체로 명확하다. 체크포인트 저장 실패가 상태 실패로 이어지지 않는 문제를 새로 확인했으며, 기존 Niagara 수명·스포너 처치 기록·InitialState 복원 제약도 남아 있다. 장치·스캐너·스폰·체크포인트·이동/연출 태스크의 C++ 핵심 경로와 필요한 UE 5.8 구현을 정적으로 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 체크포인트 저장 실패가 StateTree 상태의 실패 판정에서 제외된다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SaveCheckpoint.cpp:18`
- **범주**: 버그/정확성
- **문제**: 생성자가 `bConsideredForCompletion=false`와 `bCanEditConsideredForCompletion=false`를 강제한다. 부활 위치 누락과 디스크 저장 실패에서는 각각 41·46행에서 `Failed`를 반환하지만, 완료 판정에서 제외된 태스크의 실패는 상태 결과에 반영되지 않는다. 따라서 저장이 실패해도 같은 상태의 다른 태스크와 정상 완료 전이는 계속 진행할 수 있고, 상태의 실패 전이로 오류를 처리할 수 없다. 로컬 UE 5.8 `StateTreeExecutionContext.cpp:3873`은 `IsConsideredForCompletion`인 경우에만 반환값을 상태 결과와 합친다. 엔진의 `FStateTreeTest_TasksCompletion_IneligibleTaskEnterStateFail`도 이러한 `EnterState` 실패를 무시하고 트리가 `Running`으로 유지되는 것을 명시한다. 현재 구현은 Task의 실패 처리 설명인 “기록 실패는 StateTree Failed”를 상태 실패까지 보장하지 않는다.
- **제안**: 저장 태스크를 완료 판정에 포함하고, 성공 즉시 다른 병렬 태스크를 조기 완료시키지 않도록 해당 상태의 완료 정책도 함께 확인한다. 완료 판정에서 제외해야 한다면 별도 실패 이벤트/출력과 전이로 저장 실패를 명시적으로 전달한다. 저장 실패를 강제한 실행 검증에서는 이전 체크포인트가 유지되는지와 실패 경로가 선택되는지를 함께 확인한다.
- **확신도**: 높음. 프로젝트 반환값과 엔진의 판정 구현·회귀 테스트를 대조했다. 프로젝트 StateTree 에셋의 개별 태스크 완료 디스패처 구성과 실제 저장 실패 실행은 확인하지 않았다.

### 2. 🟡 상태를 떠났다 다시 들어오면 루프 Niagara가 중복 생성된다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25`
- **범주**: 버그/정확성
- **문제**: 중복 방지는 태스크 인스턴스의 `SpawnedComponent`만 확인하며, 태스크에 `ExitState` 정리가 없다. 엔진은 활성 경로에서 빠진 상태의 인스턴스 데이터를 제거하고 새 진입 때 기본값으로 다시 만든다(UE 5.8 `StateTreeExecutionContext.cpp:2680`, `:2717`). 따라서 루프 FX를 생성한 상태를 A→B→A로 왕복하면 이전 FX는 남고 포인터는 초기화되어 한 벌 더 생성된다. 부착하지 않는 44행 경로는 `SpawnSystemAtLocation`이 컴포넌트를 `WorldSettings` 소유로 만들므로 장치 셀 언로드만으로 정리되지도 않는다(엔진 `NiagaraFunctionLibrary.cpp:133`). 헤더 38행의 루프 FX 유지 설명을 이 인스턴스 가드만으로 보장할 수 없다.
- **제안**: 상태 수명에 묶는 FX라면 이탈 때 정리한다. 상태를 넘어 계속 유지할 FX라면 장치 소유 컴포넌트 등 상태 인스턴스 밖에서 핸들과 수명을 관리해 기존 FX를 재사용한다. A→B→A 및 장치 언로드·재로드에서 컴포넌트 수를 확인한다.
- **확신도**: 높음. 루프 FX를 사용하는 상태가 실제로 이탈·재진입한다는 조건의 동작을 C++와 엔진 소스로 확인했다. 특정 에셋에서 그 조건이 발생하는지는 이번 범위 밖이다.

### 3. 🟡 bNeverRevive의 처치 기록은 스포너 셀 재로드를 넘지 못한다

- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h:68`
- **범주**: 설계/구조
- **문제**: `bIsKilled`는 초기값이 false인 액터 런타임 멤버이며 외부 저장소가 없다. `bNeverRevive`는 `WxSpawner.cpp:88`의 `Respawn()`에서 이 값만 검사한다. 셀이 언로드되어 스포너 인스턴스가 사라지고 다시 로드되면 처치 여부가 초기화되고, Auto 모드의 `BeginPlay`(`:111`)가 다시 스폰한다. 그러므로 공개 API의 “영구 처치”(`WxSpawner.h:35`)는 동일한 스포너 인스턴스에만 유효하다. 해당 스포너를 공간 로딩에서 제외하는 강제 장치도 없다. `WaitSpawnersKilled` 역시 재로드 후에는 처치되지 않은 것으로 판정한다. 헤더가 셀 수명 제약을 명시하므로, 스트리밍을 넘는 부활 금지가 필요한 대상의 배치·보존 정책을 결정해야 한다.
- **제안**: 영구 처치 대상은 셀 밖의 월드 상태 저장소에 안정적인 식별자로 기록하거나, 해당 스포너를 공간 로딩에서 제외하도록 검증·강제한다. 세션/디스크 영속성까지 확대할지는 별도로 정한다. 범위를 동일 인스턴스의 `Respawn()` 억제로만 유지한다면 “영구 처치” 계약을 그 범위로 명확히 제한한다.
- **확신도**: 중간. 코드상 기록 수명과 재생성 경로는 확정적이다. 실제 보스 배치의 스트리밍 여부나 영구 처치의 게임 기획 범위는 이번에 검증하지 않았다.

### 4. 🟡 다른 도메인 태스크는 서버의 InitialState 복원 전이를 실제 진입으로 본다 — 보류

- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:60`
- **범주**: 설계/구조
- **문제**: 서버는 루트에서 트리를 시작한 뒤 `InitialState`로 복원 전이를 요청한다. 엔진은 틱 밖 전이 요청에 활성 상태의 `SourceStateID`를 채운다(UE 5.8 `StateTreeExecutionContext.cpp:2228`). WxInventory `WxStateTreeTask_GiveRewards.cpp:23`와 `WxStateTreeTask_RefillItemCharges.cpp:23`은 `SourceStateID` 무효만 복원으로 보므로, 이런 태스크를 InitialState 목적 상태에 두면 시작 시 실제 효과를 실행할 수 있다. WxWorld 자체의 태스크는 `IsRestoring`을 보지만 다른 도메인은 이 컴포넌트의 `bRestoringState`를 알지 못한다. 특정 현행 에셋에서 보상이 지급된다고 단정하지 않는다.
- **제안**: 현재 저작 규칙인 “그 상태에 일회성 효과를 두지 않는다”를 유지한다(`WxDeviceStateTreeComponent.h:74`). 2026-09-24 사용자 결정의 근거에 따라 **엔진이 컴포넌트에 시작 상태 지정을 열 때까지 보류한다**. 순정 해결은 `FStartParameters::SelectStateOverrideArgs`로 목적 상태에서 시작하는 것이지만, 현재 `UStateTreeComponent::StartTree`는 이를 전달하지 않는다. 현시점 적용에는 시작 루틴과 공개되지 않은 `FStateTreeComponentExecutionExtension`의 복제가 필요하므로 임의로 우회하지 않는다. 컴포넌트가 시작 상태 인자를 제공하면 재개해 모든 도메인이 동일한 초기 진입 판정을 받도록 한다.
- **확신도**: 높음. 현재 컴포넌트·소비 태스크·엔진 시작/전이 경로와 기존 보류 근거를 대조했다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_WaitForTrigger.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceTriggerRule.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSaveGame.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SaveCheckpoint.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`. 공개 계약은 대응 헤더를 필요한 범위에서 확인했다.
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceComponentName.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_TriggerLinkedDevices.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`. 전체 소스의 저작권 첫 줄과 헤더의 인라인 정의를 검색했다.
- **외부 근거**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`의 서버 거리·선택지 검증과 WxInventory 두 태스크의 초기 진입 가드를 확인했다. 로컬 UE 5.8의 `StateTreeExecutionContext`(태스크 완료 판정·인스턴스 재구성·전이 소스), `StateTreeComponent::StartTree`, `NiagaraFunctionLibrary::SpawnSystemAtLocationWithParams`, `StateTreeTaskStateTest`의 완료 제외 태스크 실패 테스트를 대조했다. 엔진 테스트는 읽었으며 실행하지 않았다.
- **미검토 / 한계**: BP/WBP·StateTree·Niagara 에셋 내부 및 실제 배치·셀 로딩 범위, 멀티플레이·저장 실패·인게임 동작은 검증하지 않았다. 빌드는 실행하지 않았으며 과거 사용자 체크포인트 정상 경로 확인을 이번 실패 경로의 검증으로 확장하지 않는다. 전체 소스 파일 수는 50개지만 모든 헤더를 통독한 리뷰는 아니다.

---
*문서 기준 커밋 `39f3629a4` · 리뷰일 2026-09-25 · 소스 50파일 — `/module-review`로 갱신*
