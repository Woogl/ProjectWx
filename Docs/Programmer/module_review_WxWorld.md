# WxWorld — 코드 리뷰

> 장치 상태 스냅샷 수렴·통보 기반 무틱 대기·스포너 수명이라는 세 축의 골격은 견고하고, 코딩·모듈 규칙 위반은 한 건도 없다(61개 소스 전수 확인). 직전 리뷰(`491dd7ec`, 51파일) 이후 체크포인트 서브시스템·스포너 라이브러리·`FWxDeviceExecutionPolicy`·장치 자동화 테스트 10종이 들어오면서 상태 스냅샷 모델이 크게 정리됐고, 남은 문제는 여전히 링크 서브트리·대상 소실·부착 액터·원격 클라 같은 경계 조건에 몰려 있다. 장치 상태머신(`WxDeviceStateTreeComponent.cpp` 492줄)·상호작용 스캐너·스포너·대기 등록부와 16개 StateTree 태스크 구현을 모두 읽었고, 계약 상대인 `WxGame` 의 `AWxEnemyCharacter`·`UWxRespawnLibrary`·`UWxViewModel_InteractionList`·`AWxNpc` 와 `WxAI` 의 `UWxPatrolComponent` 까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 재생 중 상호작용자가 사라지면 몽타주 태스크가 Failed 로 끝나 장치가 죽는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:46`
- **범주**: 버그/정확성
- **문제**: 같은 파일 안에서 "관측할 수 없다"는 같은 사정을 두 갈래로 처리한다 — 진입 시 대상·몽타주가 없으면 `Succeeded`(`:30`), 틱 중 `AnimInstance` 가 없어도 `Succeeded`(`:53`) 인데, 틱 중 `InteractingCharacter` 가 사망·언포제스·파괴되면 `Failed` 다. 헤더 doc-comment(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:29`)는 "폴링은 대상이 사라진 것까지 종료로 본다"고 계약을 명시하는데 구현이 그 계약과 어긋난다. `Failed` 는 On Succeeded 전이만 저작한 상태에서 상위로 전파되어 트리 전체를 멈추고, 권위가 `RunStatus=Failed` 를 발행하면 클라도 `Super::StopLogic` 으로 따라 멈춘다(`Private/Device/WxDeviceStateTreeComponent.cpp:373`) — 그 장치는 남은 세션 내내 상호작용 불가가 된다. 몽타주 재생 중 플레이어 사망은 실제로 일어나는 경로다.
- **제안**: 틱 중 대상 소실도 나머지 경로와 같이 `Succeeded` 로 처리하고, 필요하면 Verbose 로그만 남긴다.
- **확신도**: 높음

### 2. 🟡 스포너 정리가 부착된 액터를 종류 가리지 않고 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:174`
- **범주**: 버그/정확성
- **문제**: `DestroySpawnedActor()` 는 추적 인스턴스를 지운 뒤 `GetAttachedActors()` 로 직속 부착 액터를 전부 `Destroy()` 한다(`:178`). 주석은 "약참조를 놓친 경우까지 대비한 안전망"이라 하지만, `TWeakObjectPtr` 는 대상이 GC 되기 전엔 비지 않으므로 살아 있는 인스턴스를 놓치는 경우가 사실상 없고 스폰 대상 본인은 `Existing != TrackedActor` 로 이미 걸러진다. 결국 이 순회에 실제로 걸리는 것은 디자이너가 스포너에 붙여 둔 다른 액터(조명·마커·연출 소품 등)뿐이다. `Respawn()` 은 플레이어가 죽을 때마다 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:78`)이 `TryRespawnAll` 로 Auto 스포너 전부에 부르므로, 그런 부착물은 첫 사망에 영구히 사라진다.
- **제안**: 순회를 `IWxSpawnable` 구현체로 좁히거나(`Existing->Implements<UWxSpawnable>()`), 안전망이 필요 없다면 순회를 걷어내고 추적 약참조만 쓴다.
- **확신도**: 중간

### 3. 🟡 링크 서브트리의 상태 Tag 는 발행되지만 추종할 수 없어 클라 동기화가 영구 실패한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:195`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:356`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:450`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState()` 는 `Execution->ActiveFrames` 전체를 역순으로 훑고 각 프레임의 `Frame.StateTree` 에서 상태를 꺼내므로(`:197`), 링크된 서브트리 에셋의 Tag 까지 `LastEnteredTag` 로 잡아 스냅샷에 실어 보낸다. 반면 `HasState()`·`RequestState()` 는 루트 `StateTreeRef.GetStateTree()` 한 곳에서만 핸들을 찾는다. 그래서 서브트리 태그가 발행되면 클라의 `FollowAuthorityState()` 가 `HasState(TargetTag)` 에서 걸려 `FailSynchronization("Authority tag is missing from the local root asset")` 로 빠지고(`:358`), `SyncFailure` 는 한 번 세워지면 지워지는 곳이 스냅샷 수신(`OnRep_StateSnapshot:293`)뿐이라 같은 태그가 계속 오는 동안 그 장치는 클라에서 영영 어긋난 채 남는다. 게다가 로그 문구가 "로컬 루트 에셋에 없다"여서 서버·클라 에셋 불일치로 오진하게 만든다.
- **제안**: 상태 키 계약을 루트 에셋 Tag 로 좁혀 `ObserveActiveState()` 탐색을 루트 프레임으로 한정하거나, 조회·전이 요청도 활성 프레임의 에셋까지 대칭으로 확장한다.
- **확신도**: 중간(현재 장치 에셋이 링크 서브트리를 쓰는지는 C++ 만으로 확인할 수 없다)

### 4. 🟡 '플레이어 입력 끄기'가 상호작용 입력만은 막지 못한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:49`
- **범주**: 설계/구조
- **문제**: 이 태스크는 `Pawn->DisableInput(PC)` 로 폰의 InputComponent 만 PC 입력 스택에서 내린다. 그런데 이 모듈의 상호작용 입력은 폰이 아니라 HUD 위젯이 받는다(`Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:28` — "HUD 리스트 위젯이 Enhanced Input 으로 받아 ... `TryInteractSelected` 를 호출한다"). 실제 호출부는 `UFUNCTION(BlueprintCallable)` 인 뷰모델의 `RequestInteract()`(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:86`)이며, 폰 입력 스택을 전혀 타지 않는다. 따라서 입력을 끈 상태(엘리베이터 이동·컷신)에서도 플레이어는 주변의 다른 장치·NPC 와 계속 상호작용할 수 있다. 헤더는 "폰 입력 전체를 토글"이라 적어 이 구멍이 드러나지 않는다.
- **제안**: 이미 있는 관례(스캐너의 `CanActivateInteract` 가 어빌리티의 `ActivationBlockedTags` 를 단일 소스로 삼는 방식, `WxInteractionScannerComponent.cpp:302`)를 따라, 입력을 끄는 동안 당사자 ASC 에 차단 태그를 발행해 스캐너 표시와 서버 활성이 함께 닫히게 한다.
- **확신도**: 중간

### 5. 🟡 원격 클라에서는 NPC 상호작용 후보가 성립하지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:13`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:37`
- **범주**: 설계/구조
- **문제**: 헤더가 "NPC 에겐 이 대기가 곧 열기다"라며 `IsAwaited()` 를 대상 자격의 근거로 공개하고(`Public/Interaction/WxStateTreeTask_WaitForInteraction.h:35`), 실제로 `AWxNpc::CanInteract` 가 그것만 본다(`Source/WxGame/Character/WxNpc.cpp:43`). 그런데 등록부 `InteractionWaits` 는 권위에서 구동되는 퀘스트 ST 만 채우는 프로세스 전역 정적 변수이고(`.cpp:13` 익명 namespace), 스캔·선택은 소유 클라에서만 돈다(`WxInteractionScannerComponent.cpp:35`). 리슨 호스트 본인은 권위이자 로컬이라 동작하지만, 원격 클라는 등록부가 비어 있어 NPC 가 후보 목록에 아예 뜨지 않는다 — 장치(`AWxDevice`)는 각 피어의 ST 가 `bInteractionEnabled` 를 세우므로 영향이 없어, NPC 만 조용히 빠지는 비대칭이 된다.
- **제안**: 멀티 검증 전에 결정이 필요하다 — 대기 여부를 NPC 액터의 복제 플래그로 내려 각 피어가 같은 값을 보게 하거나, "리슨 호스트 전용"임을 헤더 계약에 못박아 다음 사람이 원격 클라에서 재현될 때 헤매지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 체크포인트·부활이 `NM_Standalone` 전용인 것을 보면 싱글플레이 우선이 현재 전제로 보인다)

### 6. 🟢 IsAwaited 만 null 가드가 없고, 스캔마다 등록부 전체를 로케이터 해석한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:37`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:76`
- **범주**: 성능/안전
- **문제**: 두 가지가 겹쳐 있다. (a) 짝인 `NotifyInteracted()` 는 `Target` null 을 걸러내는데(`:27`) `IsAwaited()` 는 곧바로 `Target->GetWorld()` 를 부른다 — 둘 다 `WXWORLD_API` 로 열린 정적 진입점인데 계약이 다르다. (b) `AnyMatching`(`Public/StateTreeTask/WxStateTreeWaitRegistry.h:91`)은 등록 하나마다 술어를 돌리고, 술어 `IsWaitingFor` 는 `Wanted.SyncFind(Target->GetLevel())` 로 로케이터를 매번 해석한다(`:76`). `IsAwaited` 는 스캐너의 후보 판정 경로(초당 10회, 반경 안 NPC 수만큼)에서 불리므로 비용이 (NPC 수 × 활성 대기 수 × 경로 해석)로 곱해진다. 덧붙여 `AnyMatching` 은 조회라 죽은 등록을 걷어내지 않아, 통보가 오지 않는 동안에는 정리되지 않은 항목까지 매번 훑는다.
- **제안**: `IsAwaited()` 에 같은 null 가드를 두고, 등록 시 해석한 액터를 약참조로 함께 캐시해 조회 경로에서는 포인터 비교로 끝낸다(통보 경로만 재해석).
- **확신도**: 중간

### 7. 🟢 Public 헤더가 Private 의존 모듈의 타입을 상속한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:28`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h` 를 포함하는데 그 소속인 `GameplayStateTreeModule` 은 `PrivateDependencyModuleNames` 에 있다. 그 헤더는 다시 `UBrainComponent`(AIModule)·`IGameplayTaskOwnerInterface`(GameplayTasks) 를 상속 계층에 노출하므로 세 모듈이 모두 Public 표면에 걸려 있다. 지금은 `UWxDeviceStateTreeComponent` 가 `WXWORLD_API` 로 export 되지 않아(`Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에도 그 사실이 기록돼 있다) 밖에서 포함할 일이 없어 드러나지 않을 뿐이고, 언젠가 export 하면 소비 모듈에서 include 경로가 끊긴다.
- **제안**: 세 모듈을 `PublicDependencyModuleNames` 로 옮기거나, export 계획이 없다면 이 헤더를 `Private/` 로 내려 Public 표면에서 뺀다.
- **확신도**: 중간

### 8. 🟢 '스포너 발동'만 복구 판정을 공용 정책 대신 직접 쓴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24`
- **범주**: 중복/복잡도
- **문제**: 다른 일회성 태스크(`SendEvent`·`PlaySound`·`RespawnSpawners`·`RecordCheckpoint`·`ApplyGameplayEffectToInteractor`·`PlayLevelSequence`·`PlayInteractorMontage`)는 모두 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 을 부르는데 이 태스크만 그 정책의 앞쪽 절반인 `!Transition.SourceStateID.IsValid()` 를 직접 복제해 쓴다(정책 정의는 `Private/Device/WxDeviceExecutionPolicy.cpp:10`). 오너가 장치가 아닌 퀘스트 ST 에서는 두 식이 같은 값이라 지금은 증상이 없지만, 장치에 `InitialState` 를 지정해 시작 상태로 밀어 넣는 경로(`Private/Device/WxDeviceStateTreeComponent.cpp:248`)는 유효한 SourceStateID 를 가진 전이라 이 태스크만 복구 중에도 스포너를 실제로 발동시킨다.
- **제안**: 다른 태스크와 같이 `FWxDeviceExecutionPolicy::IsRestoring` 을 부른다.
- **확신도**: 중간

### 9. 🟢 나이아가라 태스크에 재생을 끝낼 경로가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:40`
- **범주**: 설계/구조
- **문제**: `ExitState` 가 없고 `SpawnedComponent` 를 정지시키는 곳도 없다. `SpawnSystemAttached(..., bAutoDestroy = true)` 는 재생이 끝나야 자기 자신을 지우므로 루프 FX 는 영원히 남는다 — 상태 A 에서 켠 지속 FX(화로 불꽃·기계 가동 이펙트)가 상태 B·C 로 넘어가도 계속 돈다. 헤더(`Public/StateTreeTask/WxStateTreeTask_SpawnNiagara.h:38`)는 "루프 FX 는 계속 미완료라 유지된다"고 그 사실을 서술하지만 끄는 수단은 제시하지 않아, 저작자는 지속 FX 를 상태에 묶을 방법이 없다.
- **제안**: `bStopOnExit`(기본 false) 같은 저작 스위치를 두고, 켜면 `ExitState` 에서 `Deactivate()` 로 자연 소멸시킨다. 기존 일회성 FX 저작은 기본값으로 그대로 유지된다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 10. 🟢 '애니메이션 재생'만 재선택 정책을 세우지 않아 같은 상태의 이동 태스크와 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayAnimation.h:37`
- **범주**: 버그/정확성
- **문제**: 지속형 연출 태스크 중 `ComponentMove`(`Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp:14`)·`SplineMove`(`:15`)·`PlayLevelSequence`(`:16`)는 생성자에서 `bShouldStateChangeOnReselect = false` 를 세워 재선택 시 재진입을 막는데, `FWxStateTreeTask_PlayAnimation` 만 생성자 자체가 없어 기본값(true)을 쓴다. 같은 태그로의 재진입은 실제로 일어나는 경로다 — 클라 추종이 새 EntrySerial 을 받으면 같은 태그를 Critical 로 다시 요청해 상태를 재선택시킨다(`Private/Device/WxDeviceStateTreeComponent.cpp:437`, 테스트 "New same-tag entry reselects once" `Private/Device/Tests/WxDeviceTests.cpp:311`). 애니메이션과 슬라이드를 한 상태에 함께 저작한 장치(문·엘리베이터)에서 재선택이 들어오면 애니메이션만 처음부터 다시 돌고 슬라이드는 이어져, 둘이 어긋난 채 남는다.
- **제안**: 이 태스크의 재선택 정책을 명시한다 — 이동 태스크와 맞춰 `false` 로 두거나, 재진입마다 다시 재생하는 것이 의도라면 헤더 doc-comment 에 그 이유를 적어 나머지 셋과 다른 선택임을 드러낸다.
- **확신도**: 낮음(의도된 설계일 수 있음 — `PlaySound`·`ApplyGameplayEffectToInteractor` 처럼 재진입마다 다시 울리는 쪽이 목적일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/Tests/WxDeviceTests.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RecordCheckpoint.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSubsystem.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Public/` 전체 헤더, `Private/StateTreeTask/` 의 나머지 태스크(`PlaySound`·`ApplyGameplayEffectToInteractor`·`RespawnSpawners`), `Private/Device/WxDeviceComponentName.cpp`, `Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Private/Spawnable/WxSpawnable.cpp`, `Private/System/WxWorldDeveloperSettings.cpp`, `Private/System/WxCheckpointTests.cpp`, `Private/Device/Tests/WxDeviceTestTypes.{h,cpp}`, `Private/WxWorldModule.cpp`
- **교차 확인**: `Source/WxGame/Character/WxEnemyCharacter.cpp`(`OnSpawnedBy` 자가 부착·`MarkKilled` 경로 → 2번), `Source/WxGame/Framework/WxRespawnLibrary.cpp`(사망마다 `TryRespawnAll` 이 도는 것과 체크포인트 소비 → 2번), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(`RequestInteract` 가 폰 입력 스택을 타지 않음 → 4번), `Source/WxGame/Character/WxNpc.cpp`(`IsAwaited` 소비 → 5번), `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`(`NotifyInteracted` 발신점), `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`(`ResetCheckpoint` 소비), `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`(정찰 경로를 부착 부모의 **컴포넌트**에서 찾으므로 2번의 부착 자체는 정당함을 확인)
- **미검토 / 한계**: StateTree·장치 BP 에셋의 실제 Tag·전이 조립과 WBP 내부는 범위 밖이다. 3·4·5·9·10번은 실행 재현 없이 코드 경로로만 검토했으므로 실제 배치(링크 서브트리 사용 여부, 입력을 끄는 장치가 다른 상호작용 대상과 겹치는지, 원격 클라 세션 유무, 루프 FX 저작 여부, 애니메이션과 이동을 한 상태에 묶은 장치 유무)에서 재현 조건이 성립하지 않을 수 있다. 규칙 준수는 61개 소스 전체를 검색해 확인했다 — 첫 줄 저작권 전부 존재, `WxCore` 외 Wx 플러그인 참조 없음(`WxGameplayTags`·`WxInteractable`·`WxLocatorUtils` 모두 `WxCore` 소속이며 `.uplugin` 의존도 `WxCore` 하나), `Wx` prefix 준수, `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll` 한 곳뿐(Blueprint Function Library 라 적법), `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 은 17개 파일 전부가 예외 사유 주석을 붙였고 `TWxStateTreeWaitRegistry` 도 템플릿 예외 사유를 명시했다), 델리게이트 바인딩 대상은 타이머 콜백 `HandleScanTimer` 하나이며 `Handle` prefix 를 지킨다. 람다는 스캐너의 거리 정렬 술어 하나로 캡처가 필요한 정당한 사용이다. `override` 는 `AWxSpawner::GetDefaultActorLabel`(라벨을 전면 대체하므로 `Super` 호출이 무의미)과 `UWxDeviceStateTreeComponent::IsRunning`(순정 값을 조건으로 그대로 쓴다) 외 전부 `Super` 를 부른다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 61파일 — `/module-review`로 갱신*
