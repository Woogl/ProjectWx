# WxWorld — 코드 리뷰

> 장치 동기화(서버의 태그 변경 발행, 클라의 OnRep 시점 판단, 복원 수렴)와 상호작용 경로는 확정 설계와 일치한다. 권위·널 가드도 촘촘해 모듈은 전반적으로 건강하다. 발견은 상태 이탈·재진입과 셀 스트리밍 경계에서 인스턴스 수명이 문서화된 계약과 어긋나는 곳에 몰려 있다. 소스 57파일을 모두 읽었고, Device·Interaction·Spawnable 경로는 UE 5.8 StateTree·Niagara 엔진 소스까지 따라가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 나이아가라 스폰: 상태를 떠나도 루프 FX가 남고, 다시 들어오면 한 벌 더 뜬다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25`
- **범주**: 버그/정확성
- **문제**: 중복 방지는 인스턴스 데이터 `SpawnedComponent`(헤더 `WxStateTreeTask_SpawnNiagara.h:38`)에 기댄다. 그런데 엔진은 새로 활성화되는 상태의 태스크 인스턴스 데이터를 `DefaultInstanceData`로 다시 만든다(`UE_5.8/.../StateTreeModule/Private/StateTreeExecutionContext.cpp:2680`, 공통 구간 밖은 `:2717` `ShrinkTo` 뒤에 기본값으로 다시 붙인다). 그래서 이 가드는 상태가 활성인 채 재선택될 때만 동작한다. A→B→A처럼 떠났다 돌아오면 매번 새로 스폰한다. 이 태스크에는 `ExitState`가 없어서 상태를 떠나도 루프 FX가 멈추지 않는다. 휴식 뒤 같은 상태로 돌아오는 체크포인트의 '점화' 상태 같은 장치에서는 진입할 때마다 루프 FX가 쌓인다. 부착 대상이 없을 때 쓰는 `:44` `SpawnSystemAtLocation` 경로는 컴포넌트 outer가 WorldSettings다(`NiagaraFunctionLibrary.cpp:133`). 그래서 장치 셀이 언로드돼도 FX가 남는다. 클라가 끝난 트리를 `RestartLogic`으로 되살리는 복원 경로에서도 같은 일이 생긴다. 헤더 설명 "루프 FX 는 계속 미완료라 유지되고… 다음 진입에 다시 스폰된다"는 엔진 동작과 맞지 않는다.
- **제안**: 먼저 계약을 정한다. FX가 상태에 묶여야 하면 `ExitState`에서 `SpawnedComponent->Deactivate()`를 부르고, 인스턴스 가드는 재선택 대응용으로만 남긴다. 한 번 켠 FX를 계속 유지해야 하면 인스턴스 데이터 대신 부착 컴포넌트의 자식 중 같은 에셋을 찾아 기존 FX를 판정한다. 어느 쪽이든 헤더 주석도 고친다.
- **확신도**: 중간(엔진 동작은 소스로 확인했다. 루프 FX가 있는 상태를 다시 들어가는 에셋 구성이 있는지는 확인하지 않았다)

### 2. 🟡 bNeverRevive 처치 기록이 셀 스트림 아웃과 함께 사라져 보스가 되살아난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:84`
- **범주**: 설계/구조
- **문제**: `bIsKilled`는 스포너 액터의 런타임 멤버다(`WxSpawner.h:66`). 서버에서 스포너 셀이 언로드되면 `EndPlay`(`:92`)가 스폰한 액터를 치우고 처치 기록도 함께 사라진다. 셀이 다시 로드되면 `BeginPlay`의 Auto 스폰(`:84`)이 `bIsKilled=false` 상태에서 새 인스턴스를 만든다. 그래서 `bNeverRevive` 계약(`WxSpawner.h:61` "처치 후 부활 금지(보스 등)")은 `Respawn()` 경로(`:61`)에서만 지켜지고, 스트리밍으로 다시 들어올 때는 깨진다. 같은 이유로 `스포너 처치 대기`(`WxStateTreeTask_WaitSpawnersKilled.cpp:136`)도 재로드 뒤에는 처치 전으로 판정한다. 헤더 `:65` 주석이 런타임 상태라고 밝히고 있어 일반 적이 다시 나오는 것은 의도로 보인다. 다만 보스 영구 처치와의 충돌은 어디서도 다루지 않는다.
- **제안**: 최소한 `bNeverRevive` 스포너의 처치 기록은 셀 수명 밖에 둔다. 가장 작은 변경은 에디터에서 해당 스포너를 공간 로딩에서 빼도록(`bIsSpatiallyLoaded=false`) 강제하는 것이다. 세이브 연동은 보류 중인 IWxSavable 작업에서 함께 다룬다.
- **확신도**: 중간(보스 스포너를 스트리밍 셀에 배치하는지는 확인하지 않았다)

### 3. 🟡 다른 도메인 태스크가 서버의 InitialState 적용을 실제 진입으로 본다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`(`SynchronizeAfterStart`)
- **범주**: 설계/구조
- **문제**: 서버는 트리를 루트로 시작한 뒤 `InitialState` 상태로 복원 전이를 요청한다. 이 전이는 `SourceStateID`가 있어서, 컴포넌트의 복원 표시(`UWxDeviceStateTreeComponent::IsRestoring`)를 모르는 다른 도메인 태스크는 실제 진입으로 판정한다. 예를 들어 `ST_TreasureChest`의 WxInventory `보상 지급`은 `!SourceStateID.IsValid()`만 보므로, 상자를 InitialState "열림"으로 배치하면 레벨 시작 때 서버가 보상을 지급한다. 현재 InitialState를 쓰는 배치는 `LV_DevCombat` 피스톤 하나라 발현되지 않는다. 2026-09-24 `InitialState` 필드 주석에 저작 규칙("그 상태에 일회성 효과를 두지 않는다")을 남겼다.
- **제안**: 순정 해법은 트리를 `FStartParameters::SelectStateOverrideArgs`로 지정 상태에서 시작하는 것이다. 그러면 진입이 전부 `SourceStateID` 무효가 되어 모든 도메인이 같은 판정을 쓴다. 다만 `UStateTreeComponent::StartTree`가 이 인자를 넘길 길을 주지 않아 시작 루틴과 틱 깨우기 확장(`FStateTreeComponentExecutionExtension`, 모듈 밖 미공개)을 복제해야 한다. 그래서 엔진이 컴포넌트에 시작 상태 지정을 열 때까지 보류한다(2026-09-24 사용자 결정). 복원까지 재시작으로 바꾸면 공통 부모 상태가 다시 진입되어 `나이아가라 스폰` 중복(1번)이 복원마다 드러나고, 선택 실패 시 트리가 Failed로 멈춘다.
- **확신도**: 높음(엔진 `Start`·`StartTree` 소스와 에셋 사용처를 확인했다. 열린 상자 배치 계획은 확인하지 않았다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_WaitForTrigger.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceTriggerRule.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_TriggerLinkedDevices.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`와 각 헤더. 호출 측으로 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`와 `Source/WxGame/Character/WxNpc.cpp`를, 엔진 측으로 `UStateTreeComponent`·`FStateTreeExecutionContext`(인스턴스 데이터 재구성, 재선택 처리)·`UNiagaraFunctionLibrary`를 함께 읽었다.
- **훑은 파일**: 나머지 StateTree 태스크(`PlayAnimation`·`PlayLevelSequence`·`PlaySound`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners`·`TriggerSpawners`), `WxCheckpointSubsystem`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxWorldDeveloperSettings`, `WxWorldModule`, `WxWorld.Build.cs`, `WxWorld.uplugin`. 저작권 첫 줄과 `GetInstanceDataType()`·템플릿 예외 주석은 전 파일을 확인했다.
- **미검토 / 한계**: StateTree·BP 에셋 내부(전이 구성, 태그 배치, 루프 FX 사용처, 보스 스포너 배치)는 범위 밖이다. 그래서 1·2번의 실제 발생 여부는 에셋 구성에 달려 있다. 빌드, 멀티플레이 실행, 인게임 동작은 검증하지 않았다.

---
*문서 기준 커밋 `e72c9179f` · 리뷰일 2026-09-23 · 소스 57파일 — `/module-review`로 갱신*
