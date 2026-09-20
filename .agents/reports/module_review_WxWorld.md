# WxWorld — 코드 리뷰

> 장치 상태 구동(StateTree 실행·복제·클라 수렴)과 상호작용 스캔이라는 두 어려운 지점을 실제로 풀어낸 모듈이고, 특히 `UWxDeviceStateTreeComponent` 는 진입 관측·발행·추종의 경계가 주석까지 포함해 잘 정리돼 있다. 이번 리뷰는 `.Build.cs`·전체 헤더 훑기 + `Device/`·`Interaction/`·`Spawnable/` 의 핵심 cpp 와 18개 ST 태스크 cpp 전부를 읽었고, BP·ST 에셋 내부는 보지 않았다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 스폰한 Niagara 를 아무도 멈추지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SpawnNiagara.h:58`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25-46`
- **범주**: 버그/정확성
- **문제**: 이 태스크는 `EnterState` 만 선언하고 `ExitState` 가 없다. 헤더 주석이 스스로 "루프 FX 는 계속 미완료라 유지되고" 라고 적듯 루프 시스템이 정상 사용처인데, 상태를 떠나도 `bAutoDestroy = true` 는 발동하지 않으므로 FX 가 레벨이 끝날 때까지 재생된다. 게다가 재진입 가드(`IsValid(SpawnedComponent) && !IsComplete()`)는 인스턴스 데이터에만 의존하므로, 클라가 권위 상태를 따라잡느라 `UWxDeviceStateTreeComponent::RequestState` 안에서 `Super::RestartLogic()` 을 호출할 때마다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:406`) 인스턴스 데이터가 리셋되어 같은 상태에서 루프 FX 가 중복 스폰된다(스냅샷당 최대 3회 재시도).
- **제안**: `ExitState` 에서 `SpawnedComponent->Deactivate()`(또는 `DestroyComponent()`) 후 포인터를 비운다. 상태를 떠나도 남겨야 하는 연출이 실제로 있다면 `bKeepAfterExit` 같은 저작 옵션으로 명시적으로 가른다.
- **확신도**: 중간

### 2. 🟡 NPC 상호작용 자격을 권위 전용 등록부로 판정하는데 판정 주체는 소유 클라다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h:34-35`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:26`, 호출부 `Source/WxGame/Character/WxNpc.cpp:43`
- **범주**: 설계/구조
- **문제**: `InteractionWaits` 등록부(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:11-14`)는 권위에서 구동되는 ST 만 채우는데, `IsAwaited` 를 묻는 `CanInteract` 는 소유 클라에서만 도는 스캐너가 호출한다. 리슨 호스트 본인은 서버=클라라 동작하지만, 원격 클라에서는 등록부가 영원히 비어 있어 `IsAwaited` 가 항상 false 가 되고 NPC 프롬프트가 한 번도 뜨지 않는다. README 가 "리플리케이션/권한(최대 4인)" 을 명시하므로 지원 구성과 충돌한다.
- **제안**: 대기 중임을 대상 액터의 복제 상태(예: NPC 의 복제 bool 또는 GameplayTag)로 내보내고 `CanInteract` 는 그 복제값을 읽게 한다. 또는 리슨 호스트 단독 전제를 확정하고 헤더·README 에 "원격 클라는 NPC 프롬프트를 보지 못한다" 를 못 박는다.
- **확신도**: 중간(호출부 주석이 "서버가 곧 클라인 전제" 라고 적고 있어 의식적으로 받아들인 제약일 수 있다)

### 3. 🟡 상호작용 반경이 두 모듈에 독립 필드로 중복돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:71`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h:34`
- **범주**: 설계/구조
- **문제**: 클라 표시 반경(`ScanRadius = 150.f`)과 서버 사거리 검증 반경(`ScanRadius = 150.f`)이 서로 다른 모듈의 별개 `EditDefaultsOnly` 프로퍼티이고, 동기화 보장은 주석 한 줄("서버 사거리 검증(WxAbility_Interact)의 반경과 일치시킨다")뿐이다. 한쪽만 조정하면 프롬프트는 뜨는데 서버가 조용히 거절하거나(클라가 큼), 닿는 대상이 목록에 안 뜬다(서버가 큼). 실패가 로그 없이 "입력이 씹힌다" 로만 나타나 추적이 어렵다.
- **제안**: 반경 하나를 `WxCore` 또는 DeveloperSettings 로 올려 양쪽이 같은 값을 읽게 한다. 어빌리티가 스캐너 프로퍼티를 볼 수 없으므로(의존 방향) 공용 설정이 유일한 단일 소스다.
- **확신도**: 높음

### 4. 🟡 `스포너 처치 대기` 가 권위 전용이라 선언해 놓고 권위 게이트가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:27-60`, 헤더 선언 `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_WaitSpawnersKilled.h:32`
- **범주**: 설계/구조
- **문제**: 헤더는 "처치 상태(bIsKilled)는 복제되지 않으므로 권위에서 구동되는 ST 전용" 이라고 못 박지만 코드에는 `HasAuthority()` 검사가 없다. 같은 폴더의 `FWxStateTreeTask_TriggerSpawners` 는 `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:30-34` 에서 정확히 그 게이트를 건다. `AWxDevice` 트리는 모든 피어에서 돌기 때문에, 디자이너가 이 태스크를 장치 트리에 얹으면 클라 사본은 `IsKilled()` 가 영원히 false 인 채 0.25초 스케줄 틱으로 N개 로케이터를 계속 해석한다(상태를 벗어나는 것은 복제 스냅샷이 밀어줄 때뿐).
- **제안**: `EnterState` 에서 오너가 권위가 아니면 `Running` 으로 머물되 스케줄 틱을 걸지 않거나(클라는 스냅샷이 끌고 간다), 컴파일 검증에 "권위 트리 전용" 경고를 추가한다.
- **확신도**: 중간

### 5. 🟡 `NotifyDeviceInteracted` 가 null 당사자로 수신 장치의 `InteractingCharacter` 를 덮어쓴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:72`, 호출부 `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp:47,62,85`
- **범주**: 버그/정확성
- **문제**: `SendEvent` 는 발신 장치의 `GetInteractingCharacter()` 를 무조건 전달하고, 수신 측은 그 값을 검사 없이 자기 필드에 대입한다. 발신 장치에 당사자가 없는 경우(플레이어가 건드리지 않고 연쇄 발동으로만 도는 장치, 스플라인 도착 등 자체 전이로 이벤트를 쏘는 장치)에는 수신 장치가 쥐고 있던 당사자가 null 로 지워진다. 그 뒤 수신 트리의 `상호작용자 몽타주 재생`·`상호작용자에게 이펙트 적용`·`플레이어 입력 켜기` 가 전부 대상을 잃고 조용히 노옵한다.
- **제안**: `NotifyDeviceInteracted` 에서 `Interactor` 가 유효할 때만 대입하거나, 당사자를 지우는 것이 의도인 경로를 명시 인자로 분리한다.
- **확신도**: 중간

### 6. 🟡 `AWxSpawner::EndPlay` 가 스폰 대상을 조건 없이 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:100-105`
- **범주**: 버그/정확성
- **문제**: 같은 파일 154행 주석이 밝히듯 스폰 대상은 일부러 스포너에 부착하지 않으므로 CMC 로 자유롭게 이동한다. 그런데 `EndPlay` 는 이유를 가리지 않고 `DestroySpawnedActor()` 를 부르므로, 스포너가 있는 월드파티션 셀이 스트림 아웃되면 멀리 떨어진 로드된 셀에서 플레이어와 교전 중인 폰도 그 자리에서 사라진다.
- **제안**: `EndPlayReason` 을 가려(`RemovedFromWorld` 는 스킵, `Destroyed`/`EndPlayInEditor`/`Quit` 만 정리) 스트리밍 아웃과 실제 파괴를 구분하거나, 대상이 스포너 셀 밖에 있으면 추적만 끊고 남긴다.
- **확신도**: 중간(대상을 셀 수명에 묶는 것이 의도일 수 있다)

### 7. 🟢 `OnInteracted` 의 중복 검사와 죽은 `return`
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:31-51`
- **범주**: 중복/복잡도
- **문제**: 31행의 `StateTreeComponent->IsRunning()` 은 37행 `CanInteract(Interactor)` 안(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:20`)에서 그대로 다시 검사된다. 또 46-51행의 `if (!BroadcastInteractionDelegate()) { ...; return; }` 에서 `return` 은 함수의 마지막 문장이라 아무 일도 하지 않는다.
- **제안**: 31-34행을 지우고 `CanInteract` 하나로 남긴다. 46행은 로그만 남긴다.
- **확신도**: 높음

### 8. 🟢 `SetInteractionBinding` 이 public 이라 바인딩 스택을 우회할 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h:68`
- **범주**: 설계/구조
- **문제**: 정당한 경로는 `PushInteractionBinding`/`PopInteractionBinding` 쌍뿐이고, `SetInteractionBinding` 을 직접 부르면 `ScopedInteractions` 스택과 현재 적용값이 어긋나 다음 Pop 이 엉뚱한 값을 복원한다. 현재 모듈 안팎을 통틀어 외부 호출자는 없다.
- **제안**: private 으로 내린다.
- **확신도**: 높음

### 9. 🟢 `IsAwaited` 만 `Target` null 검사를 빠뜨렸다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:34-37`
- **범주**: 성능/안전
- **문제**: 같은 등록부를 쓰는 짝인 `NotifyInteracted`(같은 파일 24-29행)는 null 을 걸러내는데 `IsAwaited` 는 곧바로 `Target->GetWorld()` 를 역참조한다. 두 함수 모두 `WXWORLD_API` 로 모듈 밖(`Source/WxGame`)에 노출돼 있어 방어 수준이 다를 이유가 없다.
- **제안**: `IsAwaited` 에도 같은 null 가드를 넣는다.
- **확신도**: 높음

### 10. 🟢 체크포인트 서브시스템을 검사 없이 역참조한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RecordCheckpoint.cpp:37-43`
- **범주**: 성능/안전
- **문제**: 바로 위에서 `GameInstance` 는 null 검사하면서 `GameInstance->GetSubsystem<UWxCheckpointSubsystem>()` 결과는 그대로 `->RecordCheckpoint(...)` 로 역참조한다. 현재는 `ShouldCreateSubsystem` 을 오버라이드하지 않아 항상 생성되므로 실제 크래시 위험은 낮지만, 방어 수준이 한 문장 안에서 갈린다.
- **제안**: 서브시스템 포인터를 지역 변수로 받아 `Marker`·`GameInstance` 와 같은 실패 분기에 합친다.
- **확신도**: 높음

### 11. 🟢 ASC 가 없는 폰이면 상호작용 게이트가 통째로 생략된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:138-145`
- **범주**: 버그/정확성
- **문제**: `if (const UAbilitySystemComponent* ASC = ...)` 구조라 ASC 가 없으면 `CanActivateInteract` 게이트를 건너뛰고 스캔·표시를 계속한다. 그런데 실제 실행은 같은 파일 124행에서 폰 ASC 로 `Event.Interact` 를 보내는 것이므로, ASC 없는 폰은 프롬프트만 뜨고 눌러도 아무 일이 일어나지 않는다. 헤더 99-101행이 선언한 "차단 조건의 단일 소스는 어빌리티" 도 이 경로에서만 깨진다.
- **제안**: ASC 가 없으면 `UpdateInRange({})` 후 return 하여 fail-closed 로 바꾼다.
- **확신도**: 중간(폰 교체 중 한두 프레임에만 노출되는 상태일 수 있다)

### 12. 🟢 `GetInstanceDataType()` 예외 주석이 가리키는 규칙 번호가 어긋나 있다
- **위치**: ST 태스크 헤더 16개 전부(예: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SendEvent.h:16`)와 `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h:10`
- **범주**: 규칙 위반
- **문제**: 예외 사유 주석은 전 파일에 빠짐없이 달려 있어 규칙 자체는 지켜졌지만, 본문이 "코딩 규칙 4 의 예외" 를 가리킨다. CLAUDE.md 에서 인라인 함수 정의 금지는 3번이고 `Plugins/WxWorld/README.md` 도 "코딩 규칙 3의 명시 예외" 라고 적는다. 미래 세션이 규칙표를 대조할 때 헛짚는다.
- **제안**: 17개 주석의 "규칙 4" 를 "규칙 3" 으로 일괄 정정한다(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_RecordCheckpoint.h:28` 은 번호 없이 적혀 있어 그대로 두어도 된다).
- **확신도**: 높음

### 13. 🟢 빈 로케이터여도 대기 등록을 그대로 올린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:44-49`
- **범주**: 버그/정확성
- **문제**: "완료될 수 없는 잘못된 조립" 이라고 경고만 찍고 `InteractionWaits.Add` 를 그대로 실행한다. `IsWaitingFor` 는 빈 로케이터에 대해 영원히 false 이므로 그 ST(주로 퀘스트 스텝)는 영구 정지한다. 증상이 Warning 로그 한 줄뿐이라 현장에서 소프트락으로 보인다.
- **제안**: 빈 지정이면 `Failed` 를 반환해 저작 실수를 전이로 드러내거나, 컴파일 검증(`Compile`)으로 올려 에셋 저장 시점에 잡는다. `FWxSpawnerLocatorUtils::ValidateSpawners` 와 같은 자리다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, 나머지 ST 태스크 12종의 `.h`/`.cpp`(`ComponentMove`·`SplineMove`·`PlayAnimation`·`PlayInteractorMontage`·`PlayLevelSequence`·`PlaySound`·`SpawnNiagara`·`EnablePlayerInput`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners`·`TriggerSpawners`), `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceExecutionPolicy.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/`(3종), `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp`. 교차 확인용으로 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/Character/WxNpc.cpp` 와 엔진 `StateTreeComponent.cpp`·`StateTreeInstanceData.h`(UE 5.8)를 읽었다.
- **미검토 / 한계**:
  - 규칙 점검은 전수 확인을 마쳤다 — 55개 소스 전부 Copyright 첫 줄 통과, `FORCEINLINE`/헤더 인라인 정의 없음(`GetInstanceDataType()` 16건은 예외 주석 확인), `Wx` 접두사 누락 없음, `BlueprintCallable` 은 `UWxSpawnerLibrary` 1건뿐(BP 함수 라이브러리라 허용), `Super::` 미호출 없음, `.Build.cs`·실제 include 모두 `WxCore` 외 Wx 플러그인 참조 없음.
  - `UWxDeviceStateTreeComponent` 의 진입 직렬 번호(`LocalEntrySerial`/`AppliedEntrySerial`/`bRequestIsLive`) 조합은 코드 추적으로만 검증했고, 실제 지연·패킷 손실 상황의 상태 수렴은 재현해 보지 않았다. `HandleBeginApplyTransition` 의 조기 `return` 이 다중 프레임(링크된 ST 에셋) 구성에서도 `bPendingReselect` 를 놓치지 않는지는 단일 프레임 기준 추론까지만 했다.
  - `#if WITH_EDITOR` 프리뷰 경로(`AWxSpawner::PostRegisterAllComponents`/`UpdateEditorPreviewFromSpawnableClass`)는 읽었으나 에디터에서 실행해 보지는 않았다.
  - BP/WBP·StateTree 에셋 내부 구조는 리뷰 범위 밖이라 보지 않았다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 55파일 — `/module-review`로 갱신*
