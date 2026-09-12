# WxWorld — 코드 리뷰

> 장치 상태의 복제·복구 모델과 스포너 수명, 상호작용 감지 경로, 저작용 StateTree 태스크군을 다시 훑었다. 규칙 위반은 한 건도 없고 모듈 경계(WxCore 외 Wx 플러그인 무참조)도 깨끗하며, 남은 지적은 대부분 복구(restore) 판정과 정리(cleanup) 범위의 일관성 문제다. 이번 리뷰는 소스 57파일 전부를 열었고 장치 동기화·스포너·스캐너 세 축의 cpp 를 라인 단위로 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 스포너 정리가 무관한 부착 액터까지 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:171-180`
- **범주**: 버그/정확성
- **문제**: 추적하던 `SpawnedActor` 를 파괴한 뒤, 직속 부착 액터를 종류·생성 주체를 가리지 않고 전부 `Destroy()` 한다. 이 경로는 `Respawn()`(`:59`)과 `EndPlay()`(`:102`) 양쪽에서 돈다. 스포너에 조명·연출·트리거 등 다른 액터를 붙여 둔 배치에서는 리스폰 한 번에 그것들이 함께 사라진다. 게다가 주석이 말하는 "약참조를 놓친 경우"는 실제로 발생하지 않는다 — 적 캐릭터는 `OnSpawnedBy` 에서 스스로 부착하지만 `SpawnTarget()` 이 곧바로 `SpawnedActor` 에 그 인스턴스를 담으므로(`:151`), 이 안전망이 잡아 주는 대상은 사실상 없고 부작용만 남는다.
- **제안**: 부착 목록 순회를 제거하고 추적 인스턴스만 정리한다. 보완 탐색을 유지하려면 "이 스포너가 만든 인스턴스"라는 명시적 표식(예: `GetOwner() == this` 검사나 전용 태그)을 요구한다.
- **확신도**: 높음

### 2. 🟡 스포너 발동이 장치의 초기 상태 복구에서도 실행된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24`
- **범주**: 버그/정확성
- **문제**: 이 태스크만 `!Transition.SourceStateID.IsValid()` 단독으로 초기 진입을 판정한다. 같은 모듈의 다른 일회성 태스크는 전부 `FWxDeviceExecutionPolicy::IsRestoring`(`Private/Device/WxDeviceExecutionPolicy.cpp:10`)을 쓰며, 그쪽은 장치의 `bRestoringState` 도 함께 본다. `InitialState` 를 지정한 장치는 시작 직후 `Synchronize` 가 `RequestState(InitialTarget)`(`Private/Device/WxDeviceStateTreeComponent.cpp:249`)으로 전이를 거는데, 그 전이는 SourceStateID 가 유효하므로 이 태스크에는 "라이브 발동"으로 보인다. 결과적으로 레벨 시작 시 권위 측에서 `Respawn()` 이 돌아 기존 스폰 대상을 파괴하고 다시 만든다.
- **제안**: 다른 일회성 태스크와 같이 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 으로 통일한다.
- **확신도**: 높음

### 3. 🟡 링크 에셋의 태그를 스냅샷에 싣지만 루트 에셋에서만 추종한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196-222`, `:451-455`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState` 는 활성 프레임 전체를 역순으로 훑어 링크 에셋 안의 태그 상태까지 `LastEnteredTag` 로 채택한다. 반면 `HasState` 와 `RequestState` 는 `StateTreeRef.GetStateTree()`(루트 에셋)에만 질의한다. 루트에 없는 태그가 스냅샷에 실리면 클라이언트의 `FollowAuthorityState` 가 `:357` 에서 `FailSynchronization` 을 찍고(Error 로그) 그 장치의 동기화가 영구 중단된다 — `SyncFailure` 는 스냅샷의 serial/tag 가 바뀔 때만 풀리므로 같은 스냅샷으로는 복구되지 않는다. 헤더가 선언한 "태그는 루트 에셋에서 유일한 상태 식별자" 계약을 관측 코드가 지키지 않는 것이 원인이다.
- **제안**: 관측 범위를 루트 에셋 프레임으로 한정하거나, 상태 식별·전이 요청에 프레임(에셋) 문맥까지 포함해 링크 에셋 태그도 되짚을 수 있게 한다.
- **확신도**: 중간

### 4. 🟡 몽타주 도중 대상 소실이 정상 종료 대신 실패로 전파된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:44-46`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상 부재(`:30`)와 틱 중 AnimInstance 부재(`:53`)는 Succeeded 인데, 틱 중 Character 부재만 Failed 다. 헤더 `:29` 는 "폴링은 대상이 사라진 것까지 종료로 본다"고 이 설계의 근거를 명시하고 있어 코드가 자기 계약을 어긴다. 성공 전이만 저작한 장치는 당사자 사망·리스폰으로 다음 상태에 못 가고 실패가 상위로 전파되며, 장치 컴포넌트는 완료 상태도 복제하므로 클라이언트까지 멈춘다.
- **제안**: 대상 소실을 Succeeded 로 통일한다.
- **확신도**: 높음

### 5. 🟡 애니메이션 재생의 복구·재선택 정책이 이동 태스크와 다르다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp:26`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayAnimation.h:33`
- **범주**: 버그/정확성
- **문제**: 헤더는 "'Component Move' 와 동일한 방침"이라 적었지만 두 태스크가 실제로 다르다. `ComponentMove` 는 `IsRestoringDevice` 일 때 목표 포즈로 스냅하고(`WxStateTreeTask_ComponentMove.cpp:37`) 재선택 시 재진입을 억제한다(`:14`, `bShouldStateChangeOnReselect = false`). `PlayAnimation` 은 복구 판정 자체가 없어 레이트조인·초기 상태 복원에서도 처음부터 재생하고, 재선택 억제 설정도 없어 같은 상태가 재선택될 때마다 애니메이션이 되감긴다. 두 태스크를 한 상태에 조합한 문·엘리베이터는 몸통은 열린 채로 스냅되는데 애니메이션은 여는 동작을 다시 트는 그림이 된다.
- **제안**: 복구 시 끝 포즈 적용 여부와 재선택 시 재생 여부를 명시적으로 정해 `ComponentMove` 와 맞추거나, 현 동작을 유지할 경우 헤더의 "동일한 방침" 서술을 정정한다.
- **확신도**: 중간

### 6. 🟡 폰 입력 차단이 HUD 상호작용 경로를 덮지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:49`
- **범주**: 설계/구조
- **문제**: `DisableInput` 은 폰에 걸린 입력만 끈다. 상호작용 요청은 HUD 위젯 → `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:85` → 스캐너 `TryInteractSelected`(`Private/Interaction/WxInteractionScannerComponent.cpp:69`) → ServerInteract 로 흘러 폰 입력을 한 번도 거치지 않는다. 컷신처럼 "조작 전체 금지"를 의도해 이 태스크를 쓰면 상호작용은 계속 가능하다. 실질적인 차단은 스캐너가 매 스캔에서 묻는 상호작용 어빌리티의 `CanActivateAbility`(`:302-320`)뿐인데, 이 태스크는 그 판정에 영향을 주는 어떤 태그도 붙이지 않는다.
- **제안**: 전체 조작 금지가 목적이라면 서버 어빌리티와 로컬 스캐너가 함께 보는 차단 상태(태그)를 태스크 수명 동안 유지하도록 넓힌다. 폰 입력만 끄는 것이 의도라면 헤더의 "입력 전체" 표현을 좁혀 오용을 막는다.
- **확신도**: 중간

### 7. 🟢 루프 Niagara 를 상태 이탈 시 끌 수단이 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:40-45`
- **범주**: 설계/구조
- **문제**: 태스크가 컴포넌트를 생성하지만 `ExitState` 정리 경로가 없다(헤더에도 선언 없음). 자동 파괴는 시스템 완료를 전제하므로 루프 FX 는 상태를 떠나도 계속 재생된다. 상태에 묶인 지속 효과를 저작할 방법이 현재 없다.
- **제안**: 기존 단발 효과 동작을 기본값으로 두고, 상태 이탈 시 `Deactivate` 하는 선택 옵션을 추가한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 8. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h`(GameplayStateTreeModule)를 포함하는데 그 모듈은 Private 의존성이다. 지금은 모듈 밖에서 이 헤더를 포함하는 곳이 없어 문제가 드러나지 않지만, 소비 모듈이 생기면 자기 쪽에 별도 의존성 선언이 없는 한 포함 경로가 보장되지 않는다. 클래스를 export 하지 않는 것 자체는 `Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에 근거가 적혀 있어 의도된 선택으로 본다.
- **제안**: 외부 소비가 없다면 헤더를 Private 로 옮겨 배치와 의도를 맞춘다. 열 계획이 있다면 `GameplayStateTreeModule` 을 Public 의존성으로 승격한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 태스크·시스템 파일(`WxStateTreeTask_PlaySound`, `WxStateTreeTask_RecordCheckpoint`, `WxStateTreeTask_RespawnSpawners`, `WxStateTreeTask_ApplyGameplayEffectToInteractor`, `WxSpawnable`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxCheckpointSubsystem`, `WxWorldDeveloperSettings`, `WxWorldModule`)과 대응 헤더 전부. 소비 측 대조로 `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxEditor/WxDeviceLinkVisualizer.*`, `Plugins/WxAI/.../WxPatrolComponent.cpp` 를 읽었고, 엔진 쪽은 `UStateTreeComponent::RestartLogic`/`StartTree` 와 `FStateTreeInstanceData::GetExecutionState` 의 널 여부를 확인했다.
- **규칙 점검 결과**: 소스 57파일 전부 Copyright 첫 줄 존재, `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType`·`TWxStateTreeWaitRegistry` 는 각 지점에 예외 사유 주석 있음), `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll` 한 건으로 BP Function Library 예외에 부합, 람다는 `WxInteractionScannerComponent.cpp:182` 의 정렬 술어 하나로 불가피, 델리게이트 콜백은 `HandleScanTimer`·`HandleBeginApplyTransition` 모두 `Handle` prefix, 의존성은 `WxCore` 외 Wx 플러그인 무참조. 위반 없음.
- **미검토 / 한계**: C++ 정적 검토이며 빌드·PIE 재현·StateTree/BP/WBP 에셋 검증은 하지 않았다. 3번(링크 에셋 태그)과 5번(문·엘리베이터 조합)은 실제 저작된 장치 ST 에셋이 그 구성을 쓰는지에 따라 체감 영향이 갈린다. 스캐너의 `AllObjects` 오버랩(반경 150cm, 0.1초 주기)과 `FWxStateTreeTask_WaitForInteraction::IsAwaited` 의 스캔당 `SyncFind` 재해석은 비용을 재 보지 않았고 규모가 작다고 판단해 결과에 싣지 않았다. `FWxStateTreeTask_SendEvent` 가 내장 장치 미해석 시 권위에서만 Failed 를 돌려 피어 간 진행이 한 틱 갈리는 점은 스냅샷 추종으로 수렴해 제외했다. 기존 리뷰의 "공개 헤더 export 정책" 지적은 `WxEditor` 쪽에 근거가 명시된 의도적 선택으로 확인되어 8번에서 의존성 부분만 남겼다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 57파일 — `/module-review`로 갱신*
