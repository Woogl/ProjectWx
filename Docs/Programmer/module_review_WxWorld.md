# WxWorld — 코드 리뷰

> 전반적으로 건강하다. 특히 장치 상태 권위·복제·복원 수렴(`UWxDeviceStateTreeComponent`)은 실패 경로마다 근거 주석과 가드가 붙어 있고, 프로젝트 코딩·모듈 규칙 위반은 한 건도 발견되지 않았다. 이번 리뷰는 `WxWorld.Build.cs`·`.uplugin` 과 Public/Private 헤더 전체를 훑고, 장치 상태머신·상호작용 스캐너·스포너·대기 등록부·주요 StateTree 태스크의 cpp 를 깊게 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `GetPrompts()` 가 죽은 약참조 자리를 건너뛰어 선택 인덱스와 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:77`
- **범주**: 버그/정확성
- **문제**: 바로 위 주석은 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 선언하지만, 빈 자리 채우기는 `IWxInteractable` 캐스트 실패에만 적용되고 `if (AActor* Actor = Weak.Get())` 가 거짓이면 항목 자체가 빠진다. `SelectedIndex` 는 `InRangeActors` 의 인덱스인데 프롬프트 배열만 짧아지므로 둘이 어긋난다. `UpdateInRange` 안에서는 죽은 항목을 먼저 제거한 뒤 호출하니 안전하지만, 이 함수는 public 이라 뷰모델이 초기 시드로 임의 시점(스캔 주기 0.1초 사이에 대상 액터가 파괴·스트리밍 아웃된 직후)에 부를 수 있고, 그때 HUD 는 다른 대상의 문구를 선택 항목으로 보여준다.
- **제안**: `Prompts.Add(...)` 를 널 체크 밖으로 빼서 `Weak.Get()` 이 널이면 `FText::GetEmpty()` 로 자리를 채운다(주석이 이미 그 계약을 말하고 있다).
- **확신도**: 높음

### 2. 🟡 런타임 생성 장치는 `SaveId` 가 확정되지 않아 세이브에서 통째로 빠진다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:32`
- **범주**: 설계/구조
- **문제**: `SaveId` 를 채우는 유일한 경로가 `#if WITH_EDITOR` 의 `PreSave` → `GetActorGuid()` 다. 레벨 배치 장치에는 맞지만, 이 모듈이 명시적으로 지원하는 **내장 장치(`UChildActorComponent` 로 심긴 `AWxDevice`)** 는 런타임에 새로 만들어지고 엔진이 템플릿 액터의 `ActorGuid` 를 로드 시 무효화하므로(`Actor.cpp` 의 `IsTemplate()` 분기) `SaveId` 가 영영 무효다. `UWxSaveWorldSubsystem::CaptureActor`/`RestoreActor` 는 `TActorIterator` 로 자식 액터까지 훑으므로(`Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:141`, `:310`, `:384`) 이 장치들은 저장·복원에서 조용히 제외되고, 저장할 때마다 장치 수만큼 경고 로그가 쌓인다. 즉 `FWxStateTreeTask_SendEvent` 의 Child 갈래(`Private/Device/WxStateTreeTask_SendEvent.cpp:77`)로 조립한 내장 장치는 StateTag 영속을 얻지 못한다.
- **제안**: 내장 장치의 영속을 지원할 것인지 먼저 정한다. 지원한다면 `SaveId` 를 "오너 장치의 SaveId + 자기 ChildActorComponent 프로퍼티 이름" 같은 결정적 값으로 파생시키고, 지원하지 않는다면 내장 장치는 세이브 대상이 아님을 `AWxDevice`/README 에 못박아 WxSave 의 경고가 잡음이 되지 않게 한다.
- **확신도**: 중간

### 3. 🟡 `PlayInteractorMontage` 가 당사자 소실을 `Failed` 로 답해 상태가 실패 분기로 샌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayInteractorMontage.cpp:43`
- **범주**: 버그/정확성
- **문제**: 같은 "당사자가 없다"는 상황을 `EnterState`(:27)는 무해한 `Succeeded` 로, `Tick` 은 `Failed` 로 답해 비대칭이다. 그런데 `InteractingCharacter` 는 재생 도중 실제로 널이 될 수 있다 — `AWxDevice::NotifyDeviceInteracted`(`Private/Device/WxDevice.cpp:125`)와 `OnInteracted`(:78)가 `Cast<ACharacter>(Interactor)` 결과를 무조건 대입하므로, 다른 장치가 캐릭터 없는 이벤트를 밀어 넣거나 당사자가 파괴·언포제스되면 값이 지워지고 복제로 전 피어에 전파된다. 그러면 진행 중이던 몽타주 태스크가 `Failed` 를 반환해 그 상태의 완료 전이가 저작자가 예상하지 않은 실패 분기를 타고, 실패 전이를 걸어 두지 않은 장치는 그 자리에 갇힌다.
- **제안**: `Tick` 의 당사자 소실도 `EnterState` 와 같이 `Succeeded` 로 답한다(연출 대상이 사라졌으면 연출은 끝난 것이다). 실패 신호를 남기고 싶다면 로그로 남기고 반환값은 맞춘다.
- **확신도**: 중간

### 4. 🟢 하이라이트를 끌 때도 가시성으로 걸러 외곽선이 남을 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:284`
- **범주**: 버그/정확성
- **문제**: `SetActorHighlighted` 는 `IsVisible()` 인 프리미티브만 만진다. 켤 때는 옳은 필터지만(안 보이는 형상은 외곽선에 기여하지 않는다) 끌 때도 같은 필터가 걸리므로, 하이라이트가 켜진 상태에서 숨겨진 프리미티브는 `bRenderCustomDepth=true` 를 그대로 안고 남는다. 그 메시가 나중에 다시 보이면 선택되지 않았는데도 외곽선이 뜬다.
- **제안**: 가시성 검사는 `bHighlighted == true` 일 때만 적용하고, 끌 때는 모든 프리미티브를 훑어 해제한다.
- **확신도**: 중간

### 5. 🟢 `bNeverRevive` 가 Auto 모드에만 노출되어 Manual 스포너는 영구 처치를 표현할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h:65`
- **범주**: 설계/구조
- **문제**: `bNeverRevive` 의 EditCondition 이 `SpawnMode == Auto` 로 묶여 있는데, 이 플래그가 막는 것은 `Respawn()`(`Private/Spawnable/WxSpawner.cpp:66`)이고 Manual 스포너도 `FWxStateTreeTask_TriggerSpawners` 로 그 함수를 직접 호출받는다. 결과적으로 "트리거로 소환되는 보스"는 처치 뒤에도 다시 발동되면 부활한다 — 플래그의 의미(영구 처치)는 스폰 방식과 직교인데 편집 조건이 그것을 가리고 있다.
- **제안**: EditCondition 을 걷어 두 모드 모두에서 지정 가능하게 하거나, Manual 은 일부러 항상 부활 가능하다는 의도라면 그 이유를 프로퍼티 주석에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayLevelSequence.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/Public/` 전체 헤더, 나머지 태스크 cpp(`WxStateTreeTask_PlayAnimation`·`WxStateTreeTask_PlayInteractorMontage`·`WxStateTreeTask_PlaySound`·`WxStateTreeTask_SpawnNiagara`·`WxStateTreeTask_EnablePlayerInput`·`WxStateTreeTask_ApplyGameplayEffectToInteractor`·`WxStateTreeTask_RespawnSpawners`·`WxStateTreeTask_TriggerSpawners`·`WxStateTreeTask_WaitForInteraction`), `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/`, 경계 확인용으로 `Plugins/WxCore/.../WxInteractable.h`·`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`·`Plugins/WxSave/.../WxSaveWorldSubsystem.cpp`
- **규칙 점검 결과**: `Wx` prefix·첫 줄 저작권·`Handle` 콜백 prefix(`HandleScanTimer`)·`BlueprintCallable` 사용처(BP Function Library 인 `UWxSpawnerLibrary` 하나)·`Super::` 호출·`FORCEINLINE` 부재·플러그인 의존(`WxCore` 만 참조) 모두 위반 없음. 헤더의 `GetInstanceDataType()` 인라인 정의와 `TWxStateTreeWaitRegistry` 템플릿은 규칙 6 의 명시적 예외이고, 각 파일에 예외 사유 주석이 달려 있다. 람다 2건(`WxInteractionScannerComponent.cpp:186` 정렬 술어, `WxStateTreeTask_WaitForInteraction.cpp:35` 캡처 필요한 술어)은 모두 캡처·일회성이 필요한 자리라 규칙 3 위반으로 보지 않았다.
- **미검토 / 한계**: StateTree 에셋·BP 저작 내용(태그 배선, 전이 조립)은 범위 밖이라 "에셋이 규약대로 저작되었는가"는 검증하지 못했다 — 특히 상태 Tag 유일성 계약은 코드로 강제되지 않는다. 네트워크 수렴(복원·레이트조인·재진입 멀티캐스트)과 WP 셀 스트리밍 인/아웃 시나리오는 코드 독해로만 따졌고 실제 PIE 다중 클라 실행으로 검증하지 않았다. `FWxStateTreeTask_EnablePlayerInput` 의 "첫 로컬 플레이어만 토글" 동작은 헤더에 이미 한계로 명시되어 있어 발견으로 올리지 않았다.

---
*문서 기준 커밋 `49cc6a81` · 리뷰일 2026-08-27 · 소스 51파일 — `/module-review`로 갱신*
