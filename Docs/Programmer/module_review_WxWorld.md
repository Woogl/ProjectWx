# WxWorld — 코드 리뷰

> 상태 소유·복제·복원 수렴 패턴이 한 곳(`UWxDeviceStateTreeComponent`)에 모여 있고 태스크들도 진입 경로(라이브/복원)와 권위를 일관되게 가르는, 전반적으로 건강한 모듈이다. 이번 리뷰는 `*.Build.cs`·`*.uplugin`과 51개 소스를 훑고 장치 상태머신·상호작용 스캐너·스포너·대기 등록부 등 핵심 로직 cpp를 정독했다(BP/ST 에셋 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `GetPrompts()`가 죽은 약참조를 건너뛰어 선택 인덱스와 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:77-85`
- **범주**: 버그/정확성
- **문제**: 루프가 `if (AActor* Actor = Weak.Get())`로 죽은 항목을 통째로 건너뛰므로 결과 배열이 `InRangeActors`보다 짧아진다. 바로 아랫줄 주석(`:81`)이 선언한 "인덱스 정합" 계약과 어긋나며, `GetSelectedIndex()`(`:89-92`)·`GetSelectedActor()`(`:94-101`)는 여전히 `InRangeActors` 기준 인덱스를 답한다. `UpdateInRange` 안에서는 직전에 죽은 항목을 걷어내므로 드러나지 않지만, 외부 시드 경로가 그대로 맞는다 — `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43-44`가 `GetPrompts()`와 `GetSelectedIndex()`를 짝으로 읽는다. 후보 액터가 스캔 사이에 파괴된 순간(적 처치·픽업 습득) HUD가 열리면 엉뚱한 행이 선택으로 표시되거나 목록 범위를 넘는 인덱스가 들어간다.
- **제안**: 죽은 항목도 `FText::GetEmpty()`로 자리를 채우거나(주석이 말하는 그 처리), `GetPrompts()` 진입에서 `InRangeActors`의 무효 항목을 먼저 걷어낸다.
- **확신도**: 높음

### 2. 🟡 `bPendingInteractResolve`가 복원 수렴 구간을 넘어 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:140-148`, `:165-172`
- **범주**: 설계/구조
- **문제**: 이 플래그를 지우는 곳은 `PublishAuthorityState` 하나뿐인데, `SyncStateWithTree`는 `bFollowRestoredState`가 서 있는 동안 발행 경로 자체를 건너뛰고 `FollowStateTag`로 간다. 복원·초기 상태 수렴 중에도 상호작용은 성립할 수 있으므로(수렴하며 지나가는 상태의 '상호작용 켜기'가 켠다 → `WxDevice.cpp:90`이 `NotifyInteractionPending` 호출) 플래그가 그대로 남아, 수렴이 끝난 뒤 처음 도는 발행 틱에서 그때의 상태를 근거로 `Multicast_ReenterState`가 나갈 수 있다. 그 시점의 재진입 통지는 원래의 상호작용과 무관한 상태를 가리킬 수 있고, 받은 클라 전원이 그 상태의 진입 연출을 다시 재생한다.
- **제안**: 추종 구간에 들어갈 때(`NotifySaveRestored`·BeginPlay의 InitialState 경로) 또는 `FollowStateTag` 진입에서 플래그를 함께 내린다.
- **확신도**: 중간

### 3. 🟡 연출 태스크에 데디케이티드 서버 게이트가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayLevelSequence.cpp:36`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayAnimation.cpp:26`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayInteractorMontage.cpp:32`
- **범주**: 성능/안전
- **문제**: 장치 ST는 모든 피어에서 도는데 이 세 태스크는 넷모드를 보지 않는다. 데디 서버에서도 `ULevelSequencePlayer::CreateLevelSequencePlayer`가 상태 진입마다 `ALevelSequenceActor`를 스폰·평가하고(진입/이탈마다 스폰·Destroy), 싱글노드 애님·몽타주가 서버에서 재생된다 — 볼 사람이 없는 자원 소모다. 더 문제는 이들이 **완료 폴링으로 상태 진행을 게이트한다**는 점이다(`PlayAnimation.cpp:42-44`, `PlayInteractorMontage.cpp:55`). 대상 메시가 서버에서 포즈를 틱하지 않는 설정(`OnlyTickPoseWhenRendered` 등, 프로젝트에도 선례가 있다 — `Source/WxGame/Character/WxMetaHumanComponent.cpp:178`)이면 권위 트리가 그 상태에서 영영 빠져나오지 못하고, 상태가 서버 권위이므로 클라 전원이 함께 멈춘다. 같은 모듈의 `WxStateTreeTask_EnablePlayerInput.cpp:34-39`는 데디 서버를 명시적으로 노옵 처리하고 있어 방침이 일관되지 않다.
- **제안**: 세 태스크의 `EnterState`에서 `World->IsNetMode(NM_DedicatedServer)`이면 재생을 건너뛰고 곧바로 `Succeeded`를 답한다(상태 진행은 서버가 끌고, 연출은 각 클라가 로컬로 맞춘다).
- **확신도**: 중간 (자원 소모는 확실, 게이트 정지는 메시 틱 설정에 달린 조건부)

### 4. 🟢 `AWxSpawner`의 인스턴스 파기가 세 곳에 그대로 복제되어 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:60-64`, `:107-111`, `:138-144`
- **범주**: 중복/복잡도
- **문제**: `Respawn`·`OnSaveRestored`·`EndPlay`가 "`SpawnedActor`가 살아 있으면 Destroy 하고 약참조를 리셋"하는 동일 블록을 각각 들고 있다. 의미 차이가 없는 순수 복제라, 파기 규칙이 바뀌면 세 곳을 같이 고쳐야 한다.
- **제안**: `DestroySpawnedActor()` 같은 private 헬퍼 하나로 모은다.
- **확신도**: 높음

### 5. 🟢 내장(ChildActor) 장치는 세이브 신원을 얻지 못한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:29-44`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxStateTreeTask_SendEvent.h:43-45`
- **범주**: 설계/구조
- **문제**: `SaveId`를 확정하는 유일한 자리가 `WITH_EDITOR`의 `PreSave`이고 값의 출처가 `GetActorGuid()`라, 레벨에 배치된 장치만 안정된 신원을 갖는다. 하지만 'Send Event'의 `Child` 갈래는 "오너 BP에 ChildActor로 심긴 내장 장치"를 1급 패턴으로 지원하는데, 그런 장치는 런타임에 템플릿에서 스폰되므로 두 결말 중 하나가 된다 — (a) `SaveId`가 무효라 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:311-316`이 매 저장마다 경고를 남기고 그 장치를 통째로 제외하거나, (b) BP 패키지 저장 때 ChildActorTemplate에 GUID가 한 번 구워지면 그 BP의 모든 인스턴스가 같은 레코드를 공유해 서로의 상태를 덮어쓴다. 현재 `Content/WorldObject/Gimmick/` 장치 BP 중 ChildActorComponent를 쓰는 것은 없어 아직 드러나지 않았다.
- **제안**: 이 패턴을 쓰기 전에 (a)/(b) 중 어느 쪽인지 먼저 확인하고, 내장 장치가 상태를 보존해야 한다면 "오너 SaveId + 컴포넌트 이름"처럼 부모에서 파생한 안정 키를 준다. 보존이 필요 없다면 내장 장치는 저장 대상이 아님을 doc-comment에 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 `BeginPlay`가 저작 데이터인 `LinkedDevices`를 말없이 늘린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:149-161`
- **범주**: 설계/구조
- **문제**: 부착 부모가 장치면 조건 없이 `LinkedDevices`에 `AddUnique` 한다. 이 배열은 'Send Event(Linked)'가 그대로 순회하는 대상 목록이므로(`Private/Device/WxStateTreeTask_SendEvent.cpp:59-65`), 아웃라이너에서 다른 장치에 붙여 놓은 장치는 디자이너가 지정한 대상 외에 부모에게도 모든 이벤트를 보낸다. `EditInstanceOnly` 프로퍼티에 코드가 항목을 끼워 넣는 것이라 디테일 패널 값과 런타임 값이 달라 원인 추적도 어렵다.
- **제안**: 부모 지목이 필요하면 `EWxDeviceEventTarget`에 `Parent` 갈래를 두어 저작에서 명시하게 하고, 자동 추가는 걷어낸다. 자동 추가를 유지한다면 최소한 헤더의 `LinkedDevices` 주석에 "런타임에 부착 부모가 자동으로 더해진다"를 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SendEvent.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeComponentName.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp`, 그리고 대응 Public 헤더 전부
- **참고로 함께 읽은 모듈 밖 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`(서버 권위 검증 확인), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(스캐너 소비자), `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(SaveId·컴포넌트 직렬화 경로)
- **규칙 점검 결과**: `CLAUDE.md` 위반 없음 — 51개 소스 전부 첫 줄 저작권 표기, `BlueprintCallable`은 `UWxSpawnerLibrary::TryRespawnAll`(BP Function Library) 한 곳뿐, 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 15곳과 템플릿 `TWxStateTreeWaitRegistry`뿐이며 모두 예외 사유 주석이 붙어 있다. 람다는 2곳(정렬 술어·등록부 술어)으로 모두 필요한 자리다. 의존은 `WxCore`만 참조해 플러그인 규칙을 지킨다. 오버라이드의 `Super::` 호출 누락도 없다.
- **미검토 / 한계**: ST 에셋(`ST_Door`·`ST_Elevator` 등)의 상태 Tag 배선·전이 조건은 리뷰 범위 밖이라, 태스크가 상태를 어떻게 조립해 쓰는지에서 오는 문제는 못 본다. 발견 3의 "메시 포즈 틱" 전제와 발견 5의 ChildActorTemplate GUID 굽기 여부는 엔진 cpp 소스가 없어 헤더·문서 수준까지만 확인했고 실행 검증은 하지 않았다. `TWxStateTreeWaitRegistry::FinishMatching` 도중 `FinishTask`가 즉시 완료를 일으켜 ExitState가 스윕 중인 배열을 건드리는 이론적 경로는(엔진 doc-comment 상 ST 틱 안에서 통보가 올 때만 성립) 현 호출부(`MarkKilled`는 사망 처리, `NotifyInteracted`는 어빌리티 실행)에서 도달 경로를 찾지 못해 발견으로 올리지 않았다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 51파일 — `/module-review`로 갱신*
