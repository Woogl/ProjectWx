# WxWorld — 코드 리뷰

> 장치 동기화(서버의 태그 변경 발행, 클라의 OnRep 시점 판단, Critical 전이 수렴)와 복원 판정, 상호작용 경로는 오늘 확정한 설계와 일치하고 권위·널 가드도 촘촘해 모듈은 전반적으로 건강하다. 남은 발견은 상태에 묶인 연출의 수명, 셀 스트리밍 경계, 복원 판정을 거치지 않는 진입 경로에 몰려 있다. 소스 52파일을 모두 읽었고, Device·Interaction·Spawnable·연출 태스크는 UE 5.8 StateTree·Niagara·오디오·시퀀서 엔진 소스까지 따라가 확인했다.

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
- **문제**: 중복 방지는 인스턴스 데이터 `SpawnedComponent`(`WxStateTreeTask_SpawnNiagara.h:40`)에 기댄다. 그런데 엔진은 새로 활성화되는 상태의 태스크 인스턴스 데이터를 `DefaultInstanceData`로 다시 만든다(`UE_5.8/Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/Private/StateTreeExecutionContext.cpp:2682`, 공통 구간 밖은 `:2717` `ShrinkTo` 뒤에 기본값으로 다시 붙인다). 그래서 이 가드는 상태가 활성인 채 재선택될 때만 동작하고, A→B→A처럼 떠났다 돌아오면 매번 새로 스폰한다. 이 태스크에는 `ExitState`가 없어서 상태를 떠나도 루프 FX가 멈추지 않는다. 부착 대상이 없을 때 쓰는 `:44` `SpawnSystemAtLocation` 경로는 컴포넌트 outer가 WorldSettings라(`NiagaraFunctionLibrary.cpp:133`) 장치 셀이 언로드돼도 FX가 남는다. 클라가 끝난 트리를 `RestartLogic`으로 되살리는 복원 경로도 인스턴스 데이터를 새로 만들어 같은 일이 생긴다. 헤더 `:38`의 "루프 FX 는 계속 미완료라 유지되고… 다음 진입에 다시 스폰된다"는 엔진 동작과 맞지 않는다. `ST_CheckPoint`가 이 태스크로 `NS_Fire`를 띄우므로, 휴식(`Device.CheckPoint.Resting`) 뒤 `Device.CheckPoint.Lit`로 돌아올 때 이 태스크가 있는 상태가 다시 진입되면 불꽃이 쌓인다.
- **제안**: 먼저 계약을 정한다. FX가 상태에 묶여야 하면 `ExitState`에서 `SpawnedComponent->Deactivate()`를 부르고, 인스턴스 가드는 재선택 대응용으로만 남긴다. 한 번 켠 FX를 계속 유지해야 하면 인스턴스 데이터 대신 부착 컴포넌트의 자식 중 같은 에셋을 찾아 기존 FX를 판정한다. 어느 쪽이든 헤더 주석도 고친다.
- **확신도**: 중간(엔진 동작은 소스로 확인했다. `ST_CheckPoint`에서 이 태스크가 휴식 왕복 때 다시 진입하는 상태에 있는지는 확인하지 않았다)

### 2. 🟡 bNeverRevive 처치 기록이 셀 스트림 아웃과 함께 사라져 보스가 되살아난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:86`
- **범주**: 설계/구조
- **문제**: `bIsKilled`는 스포너 액터의 런타임 멤버다(`WxSpawner.h:66`). 서버에서 스포너 셀이 언로드되면 `EndPlay`(`:90`)가 스폰한 액터를 치우고 처치 기록도 함께 사라진다. 셀이 다시 로드되면 `BeginPlay`의 Auto 스폰(`:86`)이 `bIsKilled=false` 상태에서 새 인스턴스를 만든다. 그래서 `bNeverRevive` 계약(`WxSpawner.h:61` "처치 후 부활 금지(보스 등)")은 `Respawn()` 경로(`:61`)에서만 지켜지고, 스트리밍으로 다시 들어올 때는 깨진다. 같은 이유로 `스포너 처치 대기`(`WxStateTreeTask_WaitSpawnersKilled.cpp:136`)도 재로드 뒤에는 처치 전으로 판정한다. 헤더 `:65` 주석이 런타임 상태라고 밝히고 있어 일반 적이 다시 나오는 것은 의도로 보인다. 다만 보스 영구 처치와의 충돌은 어디서도 다루지 않는다. 실제로 `LV_DevCombat`(런타임 해시를 쓰는 월드 파티션 맵)에 `BP_Boss`를 스폰하는 `bNeverRevive` 스포너가 공간 로딩 해제 표시 없이 배치돼 있다.
- **제안**: 최소한 `bNeverRevive` 스포너의 처치 기록은 셀 수명 밖에 둔다. 가장 작은 변경은 에디터에서 해당 스포너를 공간 로딩에서 빼도록(`bIsSpatiallyLoaded=false`) 강제하는 것이다. 세이브 연동은 보류 중인 IWxSavable 작업에서 함께 다룬다.
- **확신도**: 중간(배치는 에셋 문자열로 확인했다. 그 맵의 셀 로딩 범위에서 보스 스포너가 실제로 언로드되는지는 확인하지 않았다)

### 3. 🟡 다른 도메인 태스크가 서버의 InitialState 적용을 실제 진입으로 본다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:60`
- **범주**: 설계/구조
- **문제**: 서버는 트리를 루트로 시작한 뒤 `InitialState` 상태로 복원 전이를 요청한다(`SynchronizeAfterStart`). 틱 밖의 전이 요청은 엔진이 루트 프레임의 활성 상태를 소스로 채우므로 `SourceStateID`가 있다. 그래서 컴포넌트의 복원 표시(`UWxDeviceStateTreeComponent::IsRestoring`)를 모르는 다른 도메인 태스크는 이 전이를 실제 진입으로 판정한다. 예를 들어 `ST_TreasureChest`의 WxInventory `보상 지급`은 `!SourceStateID.IsValid()`만 보므로, 상자를 InitialState "열림"으로 배치하면 레벨 시작 때 서버가 보상을 지급한다. `ST_CheckPoint`의 WxInventory `RefillItemCharges`도 같은 판정이다. 현재 InitialState를 쓰는 배치는 피스톤(`Device.Piston.Off`)뿐이라(`LV_DevCombat` 1개, `SiegeCannonEmplacement01` 레벨 인스턴스 12개) 발현되지 않는다. 2026-09-24 `InitialState` 필드 주석(`WxDeviceStateTreeComponent.h:74`)에 저작 규칙("그 상태에 일회성 효과를 두지 않는다")을 남겼다.
- **제안**: 순정 해법은 트리를 `FStartParameters::SelectStateOverrideArgs`로 지정 상태에서 시작하는 것이다. 그러면 진입이 전부 `SourceStateID` 무효가 되어 모든 도메인이 같은 판정을 쓴다. 다만 `UStateTreeComponent::StartTree`가 이 인자를 넘길 길을 주지 않아 시작 루틴과 틱 깨우기 확장(`FStateTreeComponentExecutionExtension`, 모듈 밖 미공개)을 복제해야 한다. 그래서 엔진이 컴포넌트에 시작 상태 지정을 열 때까지 보류한다(2026-09-24 사용자 결정). 복원까지 재시작으로 바꾸면 공통 부모 상태가 다시 진입되어 `나이아가라 스폰` 중복(1번)이 복원마다 드러나고, 선택 실패 시 트리가 Failed로 멈춘다.
- **확신도**: 높음(엔진 `Start`·`StartTree`·`RequestTransition` 소스와 에셋 사용처를 확인했다. 열린 상자 배치 계획은 확인하지 않았다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_WaitForTrigger.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceTriggerRule.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceComponentName.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_TriggerLinkedDevices.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`와 각 헤더. 호출 측으로 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`와 `Source/WxGame/Character/WxNpc.cpp`를 읽었다. 엔진 측으로 `UStateTreeComponent`(시작·틱·틱 예약), `FStateTreeExecutionContext`(인스턴스 데이터 재구성, 틱 밖 전이 요청), `FStateTreeWeakExecutionContext::FinishTask`, `UNiagaraFunctionLibrary`, `FAudioDevice::PlaySoundAtLocation`, `ALevelSequenceActor` 복제와 카메라 컷 핸들러를 확인했다.
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/`의 `PlayAnimation`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners`, `Plugins/WxWorld/Source/WxWorld/Private/System/`의 `WxCheckpointSubsystem`·`WxSpawnerLibrary`·`WxWorldDeveloperSettings`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`. 저작권 첫 줄, 인라인 정의, `GetInstanceDataType()` 예외 주석은 전 파일을 확인했고 위반이 없다. 모듈 의존은 Wx 모듈 중 WxCore뿐이다.
- **미검토 / 한계**: StateTree·BP 에셋 내부(상태 계층, 태스크 배치, 전이 구성)는 범위 밖이다. 그래서 1·2번의 실제 발생 여부는 에셋 구성과 셀 로딩 범위에 달려 있다. 에셋 사용처와 배치 값은 `Content` 바이너리 문자열 검색으로만 확인했다. 빌드, 멀티플레이 실행, 인게임 동작은 검증하지 않았다.

---
*문서 기준 커밋 `36fbb4371` · 리뷰일 2026-09-24 · 소스 52파일 — `/module-review`로 갱신*
