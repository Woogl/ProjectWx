# WxWorld — 코드 리뷰

> 서버 권위 StateTree 수렴·통보 기반 무틱 대기·스포너 수명이라는 세 축의 골격은 여전히 견고하고, 코딩·모듈 규칙 위반은 한 건도 없다. 직전 리뷰(`c486a5c7`) 이후 이 모듈에 들어온 변경은 주석 정리와 적 역할 컴포넌트화에 따른 서술 정정뿐이라(7파일 8줄) 당시 8건이 그대로 유효하며, 문제는 여전히 링크 서브트리·대상 소실·부착 액터·원격 클라 같은 경계 조건에 몰려 있다. 51개 소스 전부를 규칙 관점에서 훑고 장치 상태머신·상호작용 스캐너·스포너·대기 등록부와 13개 StateTree 태스크 구현을 깊게 봤으며, 계약 상대인 `WxCore` 의 `IWxInteractable`·`WxGame` 의 `UWxAbility_Interact`·`UWxViewModel_InteractionList`·`AWxNpc`·`UWxEnemyComponent` 까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 재생 중 상호작용자가 사라지면 몽타주 태스크가 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:45`
- **범주**: 버그/정확성
- **문제**: 같은 파일 안에서 "관측할 수 없다"는 같은 사정을 두 가지로 처리한다 — 진입 시 대상·몽타주가 없으면 `Succeeded`(`:29`), 틱 중 `AnimInstance` 가 없어도 `Succeeded`(`:52`) 인데, 틱 중 `InteractingCharacter` 가 사망·언포제스·파괴되면 `Failed` 다. 헤더 doc-comment(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:29`)는 "폴링은 대상이 사라진 것까지 종료로 본다"고 계약을 밝히는데 구현이 그 계약과 어긋난다. On Succeeded 전이만 저작한 장치 에셋은 여기서 상태가 갇히고, 실패가 상위로 전파되면 트리 자체가 멈춰 그 장치가 남은 세션 내내 죽는다.
- **제안**: 틱 중 대상 소실도 나머지 경로와 같이 `Succeeded` 로 처리하고, 필요하면 Verbose 로그만 남긴다.
- **확신도**: 높음

### 2. 🟡 스포너 정리가 부착된 액터를 종류 가리지 않고 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:173`
- **범주**: 버그/정확성
- **문제**: `DestroySpawnedActor()` 는 추적 인스턴스를 지운 뒤 `GetAttachedActors()` 로 직속 부착 액터를 전부 `Destroy()` 한다(`:178`). 주석은 "약참조를 놓친 경우까지 대비한 안전망"이라 하지만, `TWeakObjectPtr` 는 대상이 GC 되기 전엔 비지 않으므로 살아 있는 인스턴스를 놓치는 경우가 사실상 없다 — 실제로 이 순회에 걸리는 것은 스폰 대상 본인(`Source/WxGame/Character/Component/WxEnemyComponent.cpp:126` 의 자가 부착)과, 디자이너가 스포너에 붙여 둔 다른 액터(조명·마커·연출 소품 등)뿐이다. `Respawn()` 은 체크포인트 휴식마다 `UWxSpawnerLibrary::TryRespawnAll` 로 Auto 스포너 전부에 불리므로, 그런 부착물은 첫 휴식에 영구히 사라진다.
- **제안**: 순회를 `IWxSpawnable` 구현체(또는 `OnSpawnedBy` 로 자기를 등록한 대상)로 좁히거나, 안전망이 필요 없다면 순회를 걷어내고 추적 약참조만 쓴다.
- **확신도**: 중간

### 3. 🟡 링크 서브트리의 상태 Tag 는 발행되지만 추종할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:205`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:235`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:257`
- **범주**: 설계/구조
- **문제**: `GetActiveStateTag()` 는 `Context.GetActiveFrames()` 전체를 역순으로 훑고 각 프레임의 `Frame.StateTree` 에서 상태를 꺼내므로, 링크된 서브트리 에셋의 Tag 까지 `StateTagName` 으로 발행한다. 반면 `RequestState()`·`HasState()` 는 루트 `StateTreeRef.GetStateTree()` 한 곳에서만 핸들을 찾는다. 서브트리 상태가 발행되면 클라 추종은 `RequestState` 가 무효 핸들로 조용히 노옵해 영영 어긋난 채 남고(`FollowStateTag` 가 매 틱 재요청하며 Verbose 로그만 쌓인다), `OnRep_StateTagName` 은 "권위 상태를 로컬 에셋에서 찾지 못했다" 경고를 띄워 서버·클라 에셋 불일치로 오진하게 만든다.
- **제안**: 상태 키 계약을 루트 에셋 Tag 로 좁혀 `GetActiveStateTag()` 탐색을 루트 프레임으로 한정하거나, 조회·전이 요청도 활성 프레임의 에셋까지 대칭으로 확장한다.
- **확신도**: 중간

### 4. 🟡 '플레이어 입력 끄기'가 상호작용 입력만은 막지 못한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:49`
- **범주**: 설계/구조
- **문제**: 이 태스크는 `Pawn->DisableInput(PC)` 로 폰의 InputComponent 만 PC 입력 스택에서 내린다. 그런데 이 모듈의 상호작용 입력은 폰이 아니라 HUD 위젯이 받는다(`Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:28` — "HUD 리스트 위젯이 Enhanced Input 으로 받아 ... `TryInteractSelected` 를 호출한다"). 실제 호출부도 뷰모델이며(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:86`), 프로젝트에서 폰에 입력을 바인딩하는 파일은 `Source/WxGame/Character/WxPlayerCharacter.cpp:85` 하나뿐이다. 따라서 입력을 끈 상태(엘리베이터 이동·컷신)에서도 플레이어는 주변의 다른 장치·NPC 와 계속 상호작용할 수 있다. 헤더는 "폰 입력 전체를 토글"이라 적어 이 구멍이 드러나지 않는다.
- **제안**: 이미 있는 관례(`UWxAbility_Interact` 의 `ActivationBlockedTags` 에 `State.Dialogue` 를 얹는 방식)를 따라, 입력을 끄는 동안 당사자 ASC 에 차단 태그를 발행해 스캐너 표시와 서버 활성이 함께 닫히게 한다.
- **확신도**: 중간

### 5. 🟡 나이아가라 태스크에 재생을 끝낼 경로가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:40`
- **범주**: 설계/구조
- **문제**: `ExitState` 가 없고 `SpawnedComponent` 를 정지시키는 곳도 없다. `SpawnSystemAttached(..., bAutoDestroy = true)` 는 재생이 끝나야 자기 자신을 지우므로 루프 FX 는 영원히 남는다 — 상태 A 에서 켠 지속 FX(화로 불꽃·기계 가동 이펙트)가 상태 B·C 로 넘어가도 계속 돈다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SpawnNiagara.h:38`)는 이 사실을 서술하지만 끄는 수단은 제시하지 않아, 저작자는 지속 FX 를 상태에 묶을 방법이 없다.
- **제안**: `bStopOnExit`(기본 false) 같은 저작 스위치를 두고, 켜면 `ExitState` 에서 `Deactivate()` 로 자연 소멸시킨다. 기존 일회성 FX 저작은 기본값으로 그대로 유지된다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟡 원격 클라에서는 NPC 상호작용 후보가 성립하지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:35`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h:34`
- **범주**: 설계/구조
- **문제**: 헤더가 "NPC 에겐 이 대기가 곧 열기다"라며 `IsAwaited()` 를 대상 자격의 근거로 공개하고, 실제로 `AWxNpc::CanInteract` 가 그것만 본다(`Source/WxGame/Character/WxNpc.cpp:43`). 그런데 등록부는 권위에서 구동되는 퀘스트 ST 만 채우는 프로세스 전역 정적 변수(`.cpp:13` 익명 namespace)이고, 스캔·선택은 소유 클라에서만 돈다(`WxInteractionScannerComponent.h:26`). 리슨 호스트 본인은 권위이자 로컬이라 동작하지만, 원격 클라는 등록부가 비어 있어 NPC 가 후보 목록에 아예 뜨지 않는다 — 장치(`AWxDevice`)는 각 피어의 ST 가 `bInteractionEnabled` 를 세우므로 영향이 없어, NPC 만 조용히 빠지는 비대칭이 된다.
- **제안**: 멀티 검증 전에 결정이 필요하다 — 대기 여부를 NPC 액터의 복제 플래그로 내려 각 피어가 같은 값을 보게 하거나, "리슨 호스트 전용"임을 헤더 계약에 못박아 다음 사람이 원격 클라에서 재현될 때 헤매지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — `WxNpc.cpp:42` 주석이 "서버가 곧 클라인 전제"를 이미 밝히고 있다)

### 7. 🟢 Public 헤더가 Private 의존 모듈의 타입을 상속한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:28`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h` 를 포함하는데 그 소속인 `GameplayStateTreeModule` 은 `PrivateDependencyModuleNames` 에 있다. 그 헤더는 다시 `UBrainComponent`(AIModule)·`IGameplayTaskOwnerInterface`(GameplayTasks) 를 상속 계층에 노출하므로 세 모듈이 모두 Public 표면에 걸려 있다. 지금은 `UWxDeviceStateTreeComponent` 가 `WXWORLD_API` 로 export 되지 않아(`Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에도 그 사실이 기록돼 있다) 밖에서 포함할 일이 없어 드러나지 않을 뿐이고, 언젠가 export 하면 소비 모듈에서 include 경로가 끊긴다.
- **제안**: 세 모듈을 `PublicDependencyModuleNames` 로 옮기거나, export 계획이 없다면 이 헤더를 `Private/` 로 내려 Public 표면에서 뺀다.
- **확신도**: 중간

### 8. 🟢 IsAwaited 만 null 가드가 없고, 스캔마다 등록부 전체를 로케이터 해석한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:37`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h:91`
- **범주**: 성능/안전
- **문제**: 두 가지가 겹쳐 있다. (a) 짝인 `NotifyInteracted()` 는 `Target` null 을 걸러내는데(`:27`) `IsAwaited()` 는 곧바로 `Target->GetWorld()` 를 부른다 — 둘 다 `WXWORLD_API` 로 열린 정적 진입점인데 계약이 다르다. (b) `AnyMatching` 은 등록 하나마다 술어를 돌리고, 술어 `IsWaitingFor` 는 `Wanted.SyncFind(...)` 로 로케이터를 매번 해석한다(`:76`). `IsAwaited` 는 스캐너의 후보 판정 경로(초당 10회, 반경 안 NPC 수만큼)에서 불리므로 비용이 (NPC 수 × 활성 대기 수 × 경로 해석)로 곱해진다. 지금은 활성 퀘스트 스텝 수가 작아 문제가 아니지만 대기 수와 함께 선형으로 는다. 덧붙여 `AnyMatching` 은 조회라 죽은 등록을 걷어내지 않아, 통보가 오지 않는 동안에는 정리되지 않은 항목까지 매번 훑는다.
- **제안**: `IsAwaited()` 에 같은 null 가드를 두고, 등록 시 해석한 액터를 약참조로 함께 캐시해 조회 경로에서는 포인터 비교로 끝낸다(통보 경로만 재해석).
- **확신도**: 중간

### 9. 🟢 '애니메이션 재생'만 재선택 정책을 세우지 않아 같은 상태의 이동 태스크와 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayAnimation.h:38`
- **범주**: 버그/정확성
- **문제**: 지속형 연출 태스크 중 `ComponentMove`(`Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp:13`)·`SplineMove`·`PlayLevelSequence` 는 생성자에서 `bShouldStateChangeOnReselect = false` 를 세워 재선택 시 재진입을 막는데, `FWxStateTreeTask_PlayAnimation` 만 생성자 자체가 없어 기본값(true)을 쓴다. 장치 재진입은 실제로 일어나는 경로다 — `UWxDeviceStateTreeComponent::Multicast_ReenterState`(`Private/Device/WxDeviceStateTreeComponent.cpp:106`)가 클라에 같은 상태를 다시 요청한다. 애니메이션과 슬라이드를 한 상태에 함께 저작한 장치(문·엘리베이터)에서 재진입이 들어오면 애니메이션만 처음부터 다시 돌고 슬라이드는 이어져, 둘이 어긋난 채 남는다.
- **제안**: 이 태스크의 재선택 정책을 명시한다 — 이동 태스크와 맞춰 `false` 로 두거나, 재진입마다 다시 재생하는 것이 의도라면 헤더 doc-comment 에 그 이유를 적어 나머지 셋과 다른 선택임을 드러낸다.
- **확신도**: 낮음(의도된 설계일 수 있음 — `PlaySound`·`ApplyGameplayEffectToInteractor` 처럼 재진입마다 다시 울리는 쪽이 목적일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Public/` 전체 헤더, `Private/StateTreeTask/` 의 나머지 태스크(`PlaySound`·`ApplyGameplayEffectToInteractor`·`RespawnSpawners`), `Private/System/` 전체, `Private/Device/WxDeviceComponentName.cpp`, `Private/Spawnable/WxSpawnable.cpp`, `Private/WxWorldModule.cpp`
- **교차 확인**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`(서버가 `CanInteract`·사거리를 다시 판정하므로 `ServerInteract` 가 `WithValidation` 없이도 안전함을 확인 — 변조 클라 의심 기각), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(정적 `OnAnyScannerReady` 는 `AddUObject`+`Remove` 짝이 맞아 스테일 바인딩 없음, 입력 호출부 확인 → 4번), `Source/WxGame/Character/WxNpc.cpp`(`IsAwaited` 소비 → 6번), `Source/WxGame/Character/Component/WxEnemyComponent.cpp`(`OnSpawnedBy` 자가 부착·`MarkKilled` 경로 확인 → 2번), `Source/WxGame/Character/WxPlayerCharacter.cpp`(폰이 유일한 입력 바인딩 지점임을 확인 → 4번), `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`(정찰 경로를 부착 부모에서 찾으므로 2번의 부착이 의도된 것임을 확인)
- **미검토 / 한계**: StateTree·장치 BP 에셋의 실제 Tag·전이 조립과 WBP 내부는 범위 밖이다. 3·4·5·6·9번은 실행 재현 없이 코드 경로로만 검토했으므로 실제 배치(링크 서브트리 사용 여부, 입력을 끄는 장치가 다른 상호작용 대상과 겹치는지, 루프 FX 저작 여부, 원격 클라 세션 유무, 애니메이션과 이동을 한 상태에 묶은 장치 유무)에서 재현 조건이 성립하지 않을 수 있다. 규칙 준수는 51개 소스 전체를 검색해 확인했다 — 첫 줄 저작권 전부 존재, `WxCore` 외 Wx 플러그인 참조 없음(`WxGameplayTags`·`WxInteractable`·`WxLocatorUtils` 모두 `WxCore` 소속), `Wx` prefix 준수, `BlueprintCallable` 은 `UWxSpawnerLibrary` 한 곳뿐(Blueprint Function Library 라 적법), `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 은 전 태스크가 예외 사유 주석을 붙였고 `TWxStateTreeWaitRegistry` 도 템플릿 예외 사유를 명시했다), 델리게이트 바인딩 대상은 타이머 콜백 `HandleScanTimer` 하나이며 `Handle` prefix 를 지킨다. 람다는 스캐너의 거리 정렬 술어 하나로 캡처가 필요한 정당한 사용이다. `override` 는 `AWxSpawner::GetDefaultActorLabel`(라벨을 전면 대체하므로 `Super` 호출이 무의미) 외 전부 `Super` 를 부른다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 51파일 — `/module-review`로 갱신*
