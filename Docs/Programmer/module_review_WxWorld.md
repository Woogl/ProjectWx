# WxWorld — 코드 리뷰

> 「서버 권위 State 커밋 → 복제/OnRep → GimmickStateTree 진입」이라는 단일 축으로 기믹 7종이 얇게 조립되어 있고, 널 가드·권위 가드·복원(스냅/스킵) 분기가 전반적으로 성실하다. 다만 ST 노드 라이브러리의 「헤더가 선언한 계약 vs 실제 구현」 사이에 실사용 에셋까지 영향을 주는 불일치가 하나 남아 있다. 이번 리뷰는 C++ 29파일을 모두 훑고 기믹 베이스·ST 노드 라이브러리·상호작용 스캐너·스포너의 cpp 를 정독했으며, `Content/WorldObject/Gimmick/ST_*.uasset` 8개는 어떤 태스크를 참조하는지(이름 테이블)만 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `Spawn Niagara` 가 복원 게이트를 구현하지 않아 발동 FX 가 로드/스트리밍마다 재발화한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:768-791` (계약은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:479-488`)
- **범주**: 버그/정확성
- **문제**: 헤더는 "초기 진입(StateTree 시작/복원/레이트조인)이면 기본적으로 재생하지 않는다 — 발동 FX 는 발동 순간에만 울리고 복원 시엔 침묵한다"고 명시하지만, `FWxStateTreeTask_SpawnNiagara::EnterState` 구현에는 `IsInitialOrRestoreEntry` 호출도 `Instance.bPlayOnRestore` 참조도 없다 — 무조건 재생한다. 모듈 전체에서 `bPlayOnRestore` 를 읽는 곳은 `PlaySound` 의 `:730` 단 하나뿐이므로, `SpawnNiagara` 의 `bPlayOnRestore` 체크박스는 디자이너에게 노출만 되고 아무 효과가 없다. 실사용 에셋에 이미 영향이 있다: `Content/WorldObject/Gimmick/ST_AlarmConsole.uasset` 이 `WxStateTreeTask_SpawnNiagara` 를 참조하므로, Alarmed 상태로 저장된 경보 콘솔은 세이브 복원·셀 스트리밍 인·레이트조인 때마다 경보 FX 를 다시 터뜨린다(반대로 `ST_CheckPoint` 의 모닥불 지속 FX 는 이 버그 덕에 우연히 의도대로 보인다 — 고칠 때 그쪽 인스턴스의 `bPlayOnRestore` 를 `true` 로 올려야 한다).
- **제안**: `PlaySound::EnterState`(`:729-733`)와 동일하게 함수 첫머리에 `if (IsInitialOrRestoreEntry(Context, Transition) && !Instance.bPlayOnRestore) { return EStateTreeRunStatus::Succeeded; }` 를 넣고, `ST_CheckPoint` 의 모닥불 노드 인스턴스에 `bPlayOnRestore = true` 를 세팅한다.
- **확신도**: 높음

### 2. 🟡 `CommitGimmickState` 가 동일값 커밋을 걸러내지 않아 서버만 ST 를 재진입한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:47-59`
- **범주**: 설계/구조 (리플리케이션 권위 모델)
- **문제**: `CommitGimmickState` 는 `State = NewState` 뒤 무조건 `OnRep_GimmickState()` 를 직접 호출한다. 새 값이 기존 값과 같으면 복제 프로퍼티가 변하지 않아 **클라에서는 OnRep 이 발화하지 않는데, 서버에서는 상태 태그 이벤트가 다시 발행되어 ST 가 Root 를 재선택하고 같은 leaf 로 재진입**한다 — 즉 서버/클라 ST 가 비대칭으로 동작한다. 실제 호출 경로가 존재한다: `AWxElevator::OnInteracted`(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp:57-66`)는 주석에서 "이미 AtStart 면 동일값이라 노옵"이라 적었지만 노옵이 아니라 서버에서만 「문 닫기 → 이동(즉시 collapse) → 문 열기」 시퀀스를 재생한다. `AWxCheckPoint::OnInteracted`(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCheckPoint.cpp:35`)도 이미 Lit 인 모닥불을 다시 쓰면 서버에서만 Lit 을 재진입한다.
- **제안**: 단순 `if (State == NewState) return;` 가드는 체크포인트 재휴식(Lit 재진입으로 도는 `Save Game`·`Refill Item Charges`)을 죽이므로 주의. 「같은 상태의 재발동」을 상태 전이가 아닌 별도 신호로 표현하고(전용 재발동 이벤트 태그를 명시 발행하되 그 발행이 서버/클라 어느 쪽을 태우는지 못 박음), 그 외 경로에는 동일값 가드를 둔다. 최소한 기믹별로 현재 비대칭이 허용 가능한지 확정해 주석에 남길 것.
- **확신도**: 높음(메커니즘은 코드로 확정. 기믹별 허용 여부는 판단 필요)

### 3. 🟡 상호작용 스캐너가 0.1초마다 월드의 모든 액터를 순회한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:36`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:150-179`
- **범주**: 성능/안전
- **문제**: `ScanAndPush` 가 `TActorIterator<AActor>` 로 로드된 전체 액터를 훑으며 각각에 `Cast<IWxInteractable>` 를 시도하고, 이게 `ScanInterval = 0.1f` 타이머로 초당 10회 게임 스레드에서 돈다. 비용이 "주변에 상호작용 대상이 몇 개인가"가 아니라 "월드에 액터가 몇 개 로드돼 있는가"에 비례한다 — 오픈월드/월드파티션에서 로드 액터가 수천~수만이 되면 그대로 스캔 비용이 된다. 주석은 "구현 액터는 소수라 이 캐스트가 사실상의 필터"라고 하지만, 필터링 자체가 전수 순회다.
- **제안**: 등록 방식으로 뒤집는다 — `IWxInteractable` 구현 액터가 BeginPlay/EndPlay 에 월드 서브시스템 레지스트리로 자기를 등록하고 스캐너는 그 목록만 순회한다(콜리전 무관 설계를 유지할 수 있어 이쪽이 안전). 차선으로 `ScanRadius` 구 오버랩으로 후보를 1차 축소한 뒤 인터페이스 조회로 확정한다.
- **확신도**: 높음

### 4. 🟡 in-range 멤버십이 그대로면 프롬프트 문구 변경이 HUD 로 나가지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:219-231`
- **범주**: 버그/정확성
- **문제**: `OnListChanged.Broadcast(GetPrompts())` 는 `bChanged`(메시 멤버십 추가/제거)가 참일 때만 발화한다. 프롬프트는 `GetPrompts()` 가 대상에서 pull 하는 값이므로, **같은 메시가 계속 in-range 인 채 문구만 바뀌는 경우** 갱신 신호가 아예 나가지 않는다. 그런데 상태별 `Prompt` authoring 은 `Enable Interaction` 의 1급 기능이고(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:98`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:117-132`), `AWxElevator` 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h:60-61`)는 "층에 따라 문구를 바꾸려면 ST_Elevator 의 해당 상태 Enable Interaction 에서 덮는다"를 권장 배선으로 명시한다. 이 배선을 따르면 플레이어가 영역 안에 서 있는 동안 문구가 이전 상태 값으로 굳는다(범위를 벗어났다 재진입해야 갱신).
- **제안**: `AWxGimmick::SetCurrentInteractionPrompt` 가 값이 실제로 바뀔 때 로컬 통지를 내고 스캐너가 이를 구독하거나(도메인 방향상 통지 채널은 `WxCore` 의 `IWxInteractable` 쪽에 두는 편이 깔끔), 최소한 스캔마다 `GetPrompts()` 결과를 직전 스냅샷과 비교해 달라졌으면 브로드캐스트하도록 `bChanged` 조건을 확장한다.
- **확신도**: 높음(코드 경로는 확정. 현재 ST 에셋이 이 배선을 실제로 쓰는지는 미확인)

### 5. 🟡 `Enable Player Input` 이 모든 피어의 첫 로컬 플레이어를 끄고, 언와인드 경로가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:36-58`(헬퍼), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:136-144`(EnterState)
- **범주**: 설계/구조
- **문제**: 두 가지가 겹친다. (a) `SetLocalPlayerInputEnabled` 는 `GEngine->GetFirstLocalPlayerController(World)` 로 **그 머신의 첫 로컬 플레이어**를 잡는데, 기믹 ST 는 서버·전 클라가 각자 구동하므로 플레이어 B 가 컷신 트리거를 쓰면 복제 State 를 받은 플레이어 A 의 클라도 Playing 에 진입해 A 의 조작을 막는다. 스플릿스크린 2P 이상은 아예 토글 대상에서 빠진다. (b) 이 태스크에는 `ExitState` 가 없어 복구가 전적으로 "다음 상태에 `Enable Player Input(true)` 가 authoring 되어 있을 것"이라는 에셋 규약에 의존한다 — 재생 중 기믹 액터/셀이 파괴되어 ST 가 `StopLogic` 되거나 Idle 상태에 토글을 빠뜨리면 입력이 꺼진 채 남는 소프트락이다. 같은 파일의 `Move Interactor To Target` 은 정확히 이 문제를 의식해 차단 대상을 인스턴스 데이터에 기록하고 `ExitState` 에서 짝 해제하므로(`:313-318`, `:507-525`), 한 파일 안에서 안전망 수준이 갈린다.
- **제안**: `Move Interactor To Target` 과 동형으로 만든다 — 연출 대상을 명시 지정(기믹의 상호작용 당사자만 토글, 비대상 피어는 노옵)하고, `EnterState` 에서 실제로 끈 PC/Pawn 을 `TWeakObjectPtr` 로 기록해 `ExitState` 에서 그 기록 기준으로 되돌린다.
- **확신도**: 중간(현재 에셋 배선에서 소프트락 발현은 어렵지만, 다중 피어 오작동과 안전망 부재는 확정)

### 6. 🟡 `SetInteractingCharacter` 를 아무도 호출하지 않아 Interactor 계열 태스크 2개가 휴면 상태다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:61`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:396`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:559`
- **범주**: 중복/복잡도
- **문제**: `AWxGimmick::SetInteractingCharacter` 는 저장소 전체에서 호출부가 0개다(`UFUNCTION` 도 아니라 BP 에서도 호출 불가). 반면 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:54-62` 와 README 는 "자식이 `OnInteracted` 에서 `SetInteractingCharacter` → `CommitGimmickState`" 를 규약으로 명시하는데 기믹 7종 중 지키는 것이 하나도 없다. 결과적으로 복제 프로퍼티 `InteractingCharacter` 는 항상 null 이고, `Move Interactor To Target` 은 진입 즉시 `LogTemp` 경고 후 Succeeded, `Play Interactor Montage` 는 조용히 Succeeded 로 빠진다. 현재 ST 에셋 8개 중 두 태스크를 쓰는 것이 없어 발현은 없지만, 디자이너가 오늘 이 노드를 배치하면 원인을 알기 어렵게 무동작한다.
- **제안**: 배선 계획이 살아 있으면 자식마다 반복하지 말고 `AWxGimmick` 이 상호작용 진입점을 감싸 한 자리에서 당사자를 기록하도록 잇는다. 계획이 없으면 두 태스크와 `InteractingCharacter` 복제 필드·규약 주석을 함께 정리한다. 되살릴 경우 2차 이슈도 같이 볼 것 — 이 태스크는 모든 피어에서 `ACharacter::SetActorLocation` 으로 직접 이동시키는데(`:481`) CMC 의 이동 복제·서버 보정을 끄지 않아 소유 클라 rubber-banding / 시뮬레이티드 프록시 떨림 가능성이 있다.
- **확신도**: 높음(호출부 부재는 확정. 의도된 미배선일 수 있음)

### 7. 🟡 스포너가 스폰한 Character 를 자신에게 영구 Attach 한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:174`
- **범주**: 설계/구조
- **문제**: `SpawnTarget` 이 `Spawned->AttachToActor(this, KeepWorldTransform)` 로 스폰체를 스포너에 붙이는데, 실제 스폰 대상은 CMC 로 돌아다니는 `AWxEnemyCharacter`(`Source/WxGame/Character/WxEnemyCharacter.h:27`)이고 저장소 어디에도 detach 하는 코드가 없다. 루트가 attach 되어 있으면 `AActor::GatherCurrentMovement` 가 `ReplicatedMovement` 대신 `AttachmentReplication`(부모 상대 오프셋) 경로를 타므로 원격 클라에서 CMC 스무딩 경로를 벗어날 수 있고, 스포너를 옮기거나 회전시키면 AI 가 딸려 간다. 수명 관리 목적이라면 이미 `Respawn`/`EndPlay` 가 `SpawnedActor` 약참조로 명시 Destroy 하고 있어(`:47-51`, `:127-131`) attach 는 불필요하다.
- **제안**: attach 를 제거하거나(권장), 아웃라이너 그룹핑이 목적이면 에디터 전용으로 한정한다. 유지해야 한다면 `OnSpawnedBy` 또는 빙의 직후 detach 를 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — detach 부재와 대상이 Character 라는 점은 확정)

### 8. 🟢 스캐너가 폰 부재로 조기 리턴할 때 이전 하이라이트·목록을 정리하지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:126-131`
- **범주**: 버그/정확성
- **문제**: `ScanAndPush` 는 상호작용 불가 판정 시엔 `UpdateInRange({})` 로 목록·외곽선을 명시 정리하지만(`:137-141`), 폰이 없을 땐 아무것도 하지 않고 반환한다. 그 사이 `InRangeMeshes`·`SelectedIndex` 가 남아 선택 메시의 Custom Depth 외곽선이 켜진 채 유지되고 HUD 리스트도 마지막 값으로 굳는다. 사망처럼 태그 게이트가 먼저 목록을 비우는 흐름은 무해하지만, 태그 없이 폰이 사라지는 경로(폰 교체, 언포제스, 레벨 전환 대기)에서는 잔상이 남는다.
- **제안**: 조기 리턴 전에 `UpdateInRange({})` 를 호출해 두 경로의 정리 동작을 일치시킨다.
- **확신도**: 중간

### 9. 🟢 모듈 전용 로그 카테고리 없이 `LogTemp` 를 쓴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:401`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:838`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:147` (선언 부재는 `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h:8-13`)
- **범주**: 중복/복잡도
- **문제**: `WxCombat`·`WxAI`·`WxQuest`·`WxSave`·`WxDialogue` 는 모두 모듈 헤더에 `DECLARE_LOG_CATEGORY_EXTERN(LogWx*, Log, All)` 를 두는데 `WxWorld` 만 없어 진단 경고 3건이 `LogTemp` 로 섞여 나간다. 기믹/스포너 문제를 로그 필터로 격리할 수 없다.
- **제안**: `WxWorldModule.h/.cpp` 에 `LogWxWorld` 를 선언·정의하고 세 호출부를 교체한다.
- **확신도**: 높음

### 10. 🟢 `Component Move` 가 매 틱 `GetArchetype()` 을 호출한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:61-65`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:196`
- **범주**: 성능/안전
- **문제**: `GetMoveAnchor` 가 `Component->GetArchetype()` 을 호출하는데 `UObject::GetArchetype()` 은 outer 아키타입을 재귀 조회하고 이름으로 오브젝트를 찾는 경로라 상수 시간 게터가 아니다. 이걸 `Tick` 에서 매 프레임, 움직이는 컴포넌트마다 반복한다(엘리베이터면 문 2장 + 플랫폼이 동시에 도는 상황). 이미 `MoveSpeed` 를 `EnterState` 에서 1회 산출해 인스턴스 데이터에 캐시하는 구조라 목표 위치도 같이 캐시하지 않을 이유가 없다.
- **제안**: `EnterState` 에서 `Target`(또는 anchor)을 인스턴스 데이터에 캐시하고 `Tick` 은 그 값만 읽는다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCutsceneTrigger.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCheckPoint.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxDoor.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCheckPoint.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxTreasureChest.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxTreasureChest.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxAlarmConsole.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxAlarmConsole.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxSpawnConsole.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxSpawnConsole.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` — 계약 교차 확인용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp` 도 함께 읽음
- **직전 리뷰(커밋 `90aa0e6d`) 지적 재확인**: `Move Interactor To Target` 의 입력 차단 미해제 🔴 는 **해소**됐다 — 차단 대상을 `BlockedController`/`BlockedAbilitySystem` 으로 인스턴스 데이터에 기록하고 `ExitState` 가 그 기록만 근거로 해제한다(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:313-318`, `:384-386`, `:507-525`). 헤더들이 선언만 하고 설정하지 않던 `bConsideredForCompletion` 계약도 **해소**됐다(해당 문구가 헤더에서 제거되어 코드와 일치). 스캐너 리네임 이후 스테일하던 README 도 **해소**됐다. 나머지(발견 1·5·8)는 그대로 유효해 다시 실었다.
- **규칙 점검(위반 없음)**: 소스 29파일 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.` 확인 · `WxWorld.Build.cs` 의 Wx 의존은 `WxCore` 뿐(`WxTreasureChest.h:43` 의 `/Script/WxInventory.WxRewardTableRow` 는 meta 문자열이라 빌드 의존 아님) · `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll` 1곳으로 BP Function Library 예외에 해당 · `Wx` prefix 전수 준수 · 람다는 `WxInteractionScannerComponent.cpp:182` 의 정렬 술어 1개로 필수 용례 · `Super::` 미호출 override 는 전부 순수 가상 인터페이스 함수(`IWxInteractable`) 또는 엔진 ST 노드 훅이라 호출할 부모 구현이 없음
- **미검토 / 한계**: (1) `Content/WorldObject/Gimmick/ST_*.uasset` 8개는 어떤 태스크를 참조하는지만 이름 테이블로 대조했고 상태/전이 구조·완료 판정·노드별 파라미터 값은 열어보지 않았다 — 발견 1(모닥불 노드의 `bPlayOnRestore` 현재값), 2·4·5 의 실제 발현 여부는 이 배선에 달려 있다. (2) 기믹 BP 서브클래스의 디폴트값(초기 State, 할당된 ST 에셋)은 확인하지 않았다. (3) 멀티플레이 실기 검증은 하지 않았고 발견 2·6·7 의 네트워크 증상은 코드/엔진 경로 추론이다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 29파일 — `/module-review`로 갱신*
