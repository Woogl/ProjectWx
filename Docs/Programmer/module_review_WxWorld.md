# WxWorld — 코드 리뷰

> 서버 권위 StateTree 수렴·통보 기반 대기·스포너 수명이라는 세 축의 골격이 견고하고, 코딩·모듈 규칙 위반은 한 건도 없다. 직전 리뷰(`a8c6c495`)에서 지적한 스포너 로케이터 해석 비대칭과 세이브 잔재 주석은 이후 커밋으로 해소됐고, 남은 문제는 링크 서브트리·대상 소실·상태 스코프를 벗어나는 연출 같은 경계 조건에 몰려 있다. 51개 소스 전부를 규칙 관점에서 훑고 장치 상태머신·상호작용 스캐너·스포너·대기 등록부와 13개 StateTree 태스크 구현을 깊게 봤으며, 계약 상대인 `WxCore`의 `IWxInteractable`·`WxGame`의 `UWxAbility_Interact`·`UWxViewModel_InteractionList`, 그리고 엔진 `UStateTreeComponent`·`FActorLocatorFragment`·`FSoftObjectPath` 해석 경로까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 링크 서브트리의 상태 Tag 는 발행되지만 추종할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:205`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:235`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:257`
- **범주**: 설계/구조
- **문제**: `GetActiveStateTag()` 는 활성 프레임 전체를 역순으로 훑어 링크된 서브트리 에셋의 Tag 까지 `StateTagName` 으로 발행한다. 반면 `RequestState()`·`HasState()` 는 루트 `StateTreeRef.GetStateTree()` 한 곳에서만 핸들을 찾는다. 서브트리 상태가 발행되면 클라 추종은 `RequestState` 가 무효 핸들로 조용히 노옵해 영영 어긋난 채 남고(매 틱 Verbose 로그만 쌓인다), `OnRep_StateTagName` 은 "권위 상태를 로컬 에셋에서 찾지 못했다" 경고를 띄워 서버·클라 에셋 불일치로 오진하게 만든다.
- **제안**: 상태 키 계약을 루트 에셋 Tag 로 좁혀 `GetActiveStateTag()` 탐색을 루트 프레임으로 한정하거나, 조회·전이 요청도 활성 프레임의 에셋까지 대칭으로 확장한다.
- **확신도**: 중간

### 2. 🟡 재생 중 상호작용자가 사라지면 몽타주 태스크가 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:45`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상·몽타주가 없으면 `Succeeded`(같은 파일 `:29`)로 빠지는데, 재생 중 `InteractingCharacter` 가 사망·언포제스·파괴되면 `Failed` 를 반환한다. 헤더 doc-comment(`WxStateTreeTask_PlayInteractorMontage.h:29`)는 "폴링은 대상이 사라진 것까지 종료로 본다"고 계약을 밝히는데 구현이 그 계약과 어긋난다. On Succeeded 전이만 저작한 장치 에셋은 여기서 상태가 갇히고, 실패가 상위로 전파되면 트리가 멈춘다.
- **제안**: 재생 중 대상 소실도 진입 경로와 같이 `Succeeded` 로 처리하고, 필요하면 Verbose 로그만 남긴다.
- **확신도**: 높음

### 3. 🟡 '플레이어 입력 끄기'가 상호작용 입력만은 막지 못한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:49`
- **범주**: 설계/구조
- **문제**: 이 태스크는 `Pawn->DisableInput(PC)` 로 폰의 InputComponent 만 PC 입력 스택에서 내린다. 그런데 이 모듈의 상호작용 입력은 폰이 아니라 HUD 위젯이 받는다(`WxInteractionScannerComponent.h:28` — "HUD 리스트 위젯이 Enhanced Input 으로 받아 ... `TryInteractSelected` 를 호출한다"). 프로젝트에서 폰에 입력을 바인딩하는 곳은 `Source/WxGame/Character/WxPlayerCharacter.cpp` 하나뿐이므로, 입력을 끈 상태(엘리베이터 이동·컷신)에서도 플레이어는 주변의 다른 장치·NPC 와 계속 상호작용할 수 있다. 헤더는 "폰 입력 전체를 토글"이라 적어 이 구멍이 드러나지 않는다.
- **제안**: 이 프로젝트에 이미 있는 관례(`UWxAbility_Interact` 의 `ActivationBlockedTags` 에 `State.Dialogue` 를 얹는 방식)를 따라, 입력을 끄는 동안 당사자 ASC 에 차단 태그를 발행해 스캐너 표시와 서버 활성이 함께 닫히게 한다.
- **확신도**: 중간

### 4. 🟡 나이아가라 태스크에 재생을 끝낼 경로가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:40`
- **범주**: 설계/구조
- **문제**: `ExitState` 가 없고 `SpawnedComponent` 를 정지시키는 곳도 없다. `SpawnSystemAttached(..., bAutoDestroy = true)` 는 재생이 끝나야 자기 자신을 지우므로 루프 FX 는 영원히 남는다 — 상태 A 에서 켠 지속 FX(화로 불꽃·기계 가동 이펙트)가 상태 B·C 로 넘어가도 계속 돈다. 헤더(`WxStateTreeTask_SpawnNiagara.h:38`)는 이 사실을 서술하지만 끄는 수단은 제시하지 않아, 저작자는 지속 FX 를 상태에 묶을 방법이 없다.
- **제안**: `bStopOnExit`(기본 false) 같은 저작 스위치를 두고, 켜면 `ExitState` 에서 `Deactivate()` 로 자연 소멸시킨다. 기존 일회성 FX 저작은 기본값으로 그대로 유지된다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 죽은 약참조를 건너뛰어 프롬프트와 선택 인덱스가 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: `GetPrompts()` 는 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 주석을 달았지만, 그 폴백은 인터페이스 캐스트 실패에만 걸리고 약참조가 무효이면 항목 자체를 넣지 않는다. `UpdateInRange` 안에서 부를 때는 직전에 무효 항목을 걷어내 무해하지만, 뷰모델이 `GetPrompts()`/`GetSelectedIndex()` 를 각각 읽어 초기 시드로 삼는 경로(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`)에서는 스캔 사이(0.1s)에 앞쪽 대상이 파괴되면 배열이 당겨져 선택 인덱스가 다른 프롬프트를 가리킨다.
- **제안**: 무효 약참조에도 `FText::GetEmpty()` 를 넣어 `InRangeActors` 와 길이·인덱스를 항상 맞춘다.
- **확신도**: 높음

### 6. 🟢 Public 헤더가 Private 의존 모듈의 타입을 상속한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:28`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h` 를 포함하는데 그 소속인 `GameplayStateTreeModule` 은 `PrivateDependencyModuleNames` 에 있다. 그 헤더는 다시 `UBrainComponent`(AIModule)·`IGameplayTaskOwnerInterface`(GameplayTasks) 를 상속 계층에 노출하므로 세 모듈이 모두 Public 표면에 걸려 있다(엔진 헤더에서 확인). 지금은 `UWxDeviceStateTreeComponent` 가 `WXWORLD_API` 로 export 되지 않아(`Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에도 그 사실이 기록돼 있다) 밖에서 포함할 일이 없어 드러나지 않을 뿐이고, 언젠가 export 하면 소비 모듈에서 include 경로가 끊긴다.
- **제안**: 세 모듈을 `PublicDependencyModuleNames` 로 옮기거나, export 계획이 없다면 이 헤더를 `Private/` 로 내려 Public 표면에서 뺀다.
- **확신도**: 중간

### 7. 🟢 IsAwaited 만 null 대상을 막지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:37`
- **범주**: 성능/안전
- **문제**: 짝인 `NotifyInteracted()` 는 `Target` null 을 걸러내는데 `IsAwaited()` 는 곧바로 `Target->GetWorld()` 를 부른다. 둘 다 `WXWORLD_API` 로 열린 정적 진입점이고, 술어 `IsWaitingFor()` 도 대상을 무조건 역참조한다. 현재 유일한 호출부(`Source/WxGame/Character/WxNpc.cpp:43`)가 `this` 를 넘겨 실제 사고는 없지만, 두 진입점의 계약이 다르다.
- **제안**: `IsAwaited()` 에도 같은 null 가드를 둔다.
- **확신도**: 중간

### 8. 🟢 로케이터 해석 컨텍스트를 두고 두 주석이 정반대를 말한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:75`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.h:20`
- **범주**: 설계/구조
- **문제**: 전자는 "오너인 GameState 는 PersistentLevel 이라 WP 런타임 셀 안의 대상을 해석하지 못한다"고 하고, 후자는 "WP 런타임 셀 안의 대상도 이 한 번으로 닿는다"고 한다. 엔진 경로를 따라가면 후자가 맞다 — `FActorLocatorFragment::Resolve` 는 레벨 컨텍스트 해석이 실패하면 `Path.ResolveObject()` 로 떨어지고, 그것이 `UWorld::ResolveSubobject` → `ULevel::ResolveSubobject` → `UWorldPartition::ResolveSubobject` 로 이어져 셀 안 액터를 찾는다(컨텍스트는 PIE 인스턴스 선택에만 쓰인다). 전자의 주석은 실패 원인을 잘못 지목한 채 남아 있어, 다음 사람이 없는 문제를 우회하려 액터 순회 같은 장치를 다시 들일 위험이 있다.
- **제안**: `WaitForInteraction` 의 주석을 "대상의 레벨을 컨텍스트로 주면 PIE 인스턴스가 정확히 좁혀진다" 수준으로 정정한다(코드 동작은 그대로 두어도 된다).
- **확신도**: 중간

### 9. 🟢 README 가 WaitSpawnersKilled 를 대기 등록부 골격이라고 잘못 안내한다
- **위치**: `Plugins/WxWorld/README.md:34`
- **범주**: 중복/복잡도
- **문제**: "새 대기형 태스크" 항목이 `TWxStateTreeWaitRegistry` 골격을 따르는 예로 `WaitForInteraction`·`WaitSpawnersKilled` 를 나란히 든다. 그러나 `WxStateTreeTask_WaitSpawnersKilled.cpp` 는 등록부 헤더를 포함조차 하지 않고, `AddScheduledTickRequest` 기반 0.25초 저주기 폴링으로 구현돼 있다. README 는 이 모듈의 지정 진입점("여기서부터 읽어라")이라 잘못된 골격을 그대로 복사할 소지가 크다.
- **제안**: 예시를 `WaitForInteraction` 하나로 줄이고, `WaitSpawnersKilled` 는 "통보 지점이 없는 조건은 저주기 Scheduled Tick" 이라는 별도 패턴으로 적는다.
- **확신도**: 높음

### 10. 🟢 스포너의 인스턴스 정리 블록이 두 곳에 복사돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:58`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:118`
- **범주**: 중복/복잡도
- **문제**: `Respawn()` 과 `EndPlay()` 가 "추적 인스턴스 Destroy → 부착 액터 순회 Destroy → 약참조 Reset" 아홉 줄을 그대로 공유한다. 한쪽만 고치면 스트림 아웃 경로나 리스폰 경로 중 하나에서 인스턴스가 남는다.
- **제안**: 한 줄짜리 private 정리 함수로 모은다. 다만 이 프로젝트는 소규모 반복을 용인하는 쪽이므로 급하지 않다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Public/` 전체 헤더, `Private/StateTreeTask/` 의 나머지 태스크(`PlayAnimation`·`PlaySound`·`ApplyGameplayEffectToInteractor`·`RespawnSpawners`), `Private/System/` 전체, `Private/Device/WxDeviceComponentName.cpp`, `Private/Spawnable/WxSpawnable.cpp`, `Private/WxWorldModule.cpp`
- **교차 확인**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`(서버가 자격·사거리를 다시 판정하므로 `ServerInteract` 가 `WithValidation` 없이도 안전하고, 스캔 반경 150cm 가 양쪽에 같은 값으로 선언돼 있음을 확인), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(정적 `OnAnyScannerReady` 구독을 `BeginDestroy` 에서 정상 해제 — 댕글링 의심 기각), `Source/WxGame/Character/WxPlayerCharacter.cpp`(폰이 유일한 입력 바인딩 지점임을 확인 → 3번), `Plugins/WxCore/.../WxInteractable.h`(`CannotImplementInterfaceInBlueprint` 라 스캐너의 `Cast<IWxInteractable>` 가 BP 구현체를 놓칠 수 없음 — 의심 기각), 엔진 `StateTreeComponent.h`·`ActorLocatorFragment.cpp`·`SoftObjectPath.cpp`·`Level.cpp`(6·8번 근거)
- **미검토 / 한계**: StateTree·장치 BP 에셋의 실제 Tag·전이 조립과 WBP 내부는 범위 밖이다. 1·3·4번은 실행 재현 없이 코드 경로로만 검토했으므로 실제 배치(링크 서브트리 사용 여부, 입력을 끄는 장치가 다른 상호작용 대상과 겹치는지, 루프 FX 저작 여부)에서 재현 조건이 성립하지 않을 수 있다. 규칙 준수는 51개 소스 전체를 검색해 확인했다 — 첫 줄 저작권 전부 존재, `WxCore` 외 Wx 플러그인 참조 없음(역방향 참조도 없음), `Wx` prefix 준수, `BlueprintCallable` 은 `UWxSpawnerLibrary` 한 곳뿐(Blueprint Function Library 라 적법), `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 은 전 태스크가 예외 사유 주석을 붙였다), 델리게이트 바인딩 대상은 타이머 콜백 `HandleScanTimer` 하나이며 `Handle` prefix 를 지킨다. 람다는 스캐너의 거리 정렬 술어 하나로 캡처가 필요한 정당한 사용이다. 직전 리뷰(`a8c6c495`)의 🟡 "스포너 로케이터 해석 비대칭"·"처치 대기 폴링의 월드 순회" 는 커밋 `391fc2ee` 의 해석 단일화로, 🟢 "제거된 세이브·LSP 를 전제한 주석과 죽은 가드" 는 커밋 `e9630dc2` 의 주석 정리로 해소됐다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 51파일 — `/module-review`로 갱신*
