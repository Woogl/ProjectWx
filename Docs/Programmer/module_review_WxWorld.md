# WxWorld — 코드 리뷰

> 「서버 권위 State 커밋 → 복제/OnRep → GimmickStateTree 진입」이라는 단일 축으로 기믹 7종이 얇게 조립되어 있고, 널 가드·권위 가드·복원(스냅/스킵) 분기가 전반적으로 성실하다. 직전 리뷰의 지적 중 스캐너 전수 순회·`LogTemp`·매 틱 `GetArchetype()`·폰 부재 시 정리 누락은 모두 해소됐고, 남은 결함은 ST 노드 라이브러리의 「헤더가 선언한 계약 vs 실제 구현」 불일치와 리플리케이션 대칭성에 몰려 있다. 이번 리뷰는 C++ 29파일을 전부 읽고 기믹 베이스·ST 노드 라이브러리·상호작용 스캐너·스포너의 cpp 를 정독했으며, `Content/WorldObject/Gimmick/ST_*.uasset` 8개는 참조 태스크 이름만 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 `Spawn Niagara` 가 복원 게이트를 구현하지 않아 발동 FX 가 로드/스트리밍마다 재발화한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:771-794` (계약은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:483-494`)
- **범주**: 버그/정확성
- **문제**: 헤더는 "초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 기본적으로 재생하지 않는다"(`WxGimmickStateTreeNodes.h:491`)고 명시하고 디자이너용 `bPlayOnRestore` 체크박스까지 노출하지만(`:483-485`), `FWxStateTreeTask_SpawnNiagara::EnterState` 본문에는 `IsInitialOrRestoreEntry` 호출도 `Instance.bPlayOnRestore` 참조도 없다 — 조건 없이 재생한다. 모듈 전체에서 `bPlayOnRestore` 를 실제로 읽는 곳은 `PlaySound::EnterState`(`:732-736`) 하나뿐이다. 에셋 영향이 실재한다: `Content/WorldObject/Gimmick/ST_AlarmConsole.uasset` 이 이 태스크를 참조하므로, Alarmed 로 저장된 경보 콘솔은 세이브 복원·셀 스트리밍 인·레이트조인 때마다 경보 FX 를 다시 터뜨린다. 반대로 `ST_CheckPoint.uasset` 의 모닥불 지속 FX 는 이 누락 덕에 우연히 의도대로 보인다.
- **제안**: `PlaySound::EnterState`(`:732-736`)와 동형으로 첫머리에 `if (IsInitialOrRestoreEntry(Context, Transition) && !Instance.bPlayOnRestore) { return EStateTreeRunStatus::Succeeded; }` 를 넣고, 고칠 때 `ST_CheckPoint` 의 모닥불 노드 인스턴스에 `bPlayOnRestore = true` 를 세팅한다(안 하면 모닥불이 로드 후 꺼진다).
- **확신도**: 높음

### 2. 🟡 `CommitGimmickState` 가 동일값 커밋을 걸러내지 않아 서버만 ST 를 재진입한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:47-59`
- **범주**: 설계/구조 (리플리케이션 권위 모델)
- **문제**: `State = NewState` 뒤 무조건 `OnRep_GimmickState()` 를 직접 호출한다. 새 값이 기존 값과 같으면 복제 프로퍼티가 변하지 않아 **클라에서는 OnRep 이 발화하지 않는데 서버에서는 상태 태그 이벤트가 다시 발행되어 ST 가 Root 를 재선택하고 같은 leaf 로 재진입**한다 — 서버/클라 ST 가 비대칭으로 돈다. 실제 호출 경로가 있다: `AWxElevator::OnInteracted`(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp:57-66`)는 주석에 "이미 AtStart 면 동일값이라 노옵"이라 적었지만 노옵이 아니라 서버에서만 「문 닫기 → 이동(즉시 collapse) → 문 열기」를 재생한다(리슨호스트 화면에서만 문이 여닫힌다). `AWxCheckPoint::OnInteracted`(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCheckPoint.cpp:35`)도 이미 Lit 인 모닥불에서 서버만 Lit 을 재진입한다.
- **제안**: 단순 `if (State == NewState) return;` 가드는 체크포인트 재휴식(Lit 재진입으로 도는 `Save Game`·`Refill Item Charges`)을 죽이므로 주의. 「같은 상태의 재발동」을 상태 전이가 아닌 별도 재발동 이벤트 태그로 명시 발행하고(그 발행이 서버/클라 어느 쪽을 태우는지 못 박음), 그 외 경로에는 동일값 가드를 둔다. 최소한 기믹별로 이 비대칭이 허용 가능한지 확정해 주석에 남긴다.
- **확신도**: 높음(메커니즘은 코드로 확정, 기믹별 허용 여부는 판단 필요)

### 3. 🟡 in-range 멤버십이 그대로면 프롬프트 문구 변경이 HUD 로 나가지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:228-239`
- **범주**: 버그/정확성
- **문제**: `OnListChanged.Broadcast(GetPrompts())`(`:238`)는 `bChanged`(메시 멤버십 추가/제거)가 참일 때만 발화한다(`:228-231`). 프롬프트는 `GetPrompts()` 가 대상에서 pull 하는 값이라, **같은 메시가 계속 in-range 인 채 문구만 바뀌면** 갱신 신호가 아예 나가지 않는다. 그런데 상태별 `Prompt` authoring 은 `Enable Interaction` 의 1급 기능이고(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:99`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:123-138`), `AWxElevator` 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h:60`)는 "층에 따라 문구를 바꾸려면 ST_Elevator 의 해당 상태 Enable Interaction 에서 덮는다"를 권장 배선으로 명시한다. 상태 전이 중 그 영역을 끄지 않는 기믹(예: Unlit→Lit 내내 상호작용이 켜져 있는 체크포인트)이라면 플레이어가 영역 안에 서 있는 동안 문구가 이전 상태 값으로 굳는다(범위를 벗어났다 재진입해야 갱신).
- **제안**: `AWxGimmick::SetCurrentInteractionPrompt` 가 값이 실제로 바뀔 때 로컬 통지를 내고 스캐너가 구독하거나(도메인 방향상 통지 채널은 `WxCore` 의 `IWxInteractable` 쪽이 깔끔), 최소한 스캔마다 `GetPrompts()` 결과를 직전 스냅샷과 비교해 달라졌으면 브로드캐스트하도록 `bChanged` 조건을 확장한다.
- **확신도**: 높음(코드 경로는 확정. 실제 발현은 각 ST 에셋이 전이 중 그 영역을 끄는지에 달려 있어 미확인)

### 4. 🟡 `Enable Player Input` 이 모든 피어의 첫 로컬 플레이어를 끄고, 언와인드 경로가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:37-59`(헬퍼), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:137-145`(EnterState), 선언은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:115-130`
- **범주**: 설계/구조
- **문제**: 두 가지가 겹친다. (a) `SetLocalPlayerInputEnabled` 는 `GEngine->GetFirstLocalPlayerController(World)` 로 **그 머신의 첫 로컬 플레이어**를 잡는데, 기믹 ST 는 서버·전 클라가 각자 구동하므로 플레이어 B 가 컷신 트리거를 쓰면 복제 State 를 받은 플레이어 A 의 클라도 Playing 에 진입해 A 의 조작을 막는다(스플릿스크린 2P 이상은 반대로 토글에서 아예 빠진다). (b) 이 태스크에는 `ExitState` 가 없어(`h:115-130` 에 선언 자체가 없음) 복구가 전적으로 "다음 상태에 `Enable Player Input(true)` 가 authoring 되어 있을 것"이라는 에셋 규약에 의존한다 — 재생 중 기믹 액터/셀이 파괴되어 ST 가 정지되거나 Idle 상태에 토글을 빠뜨리면 입력이 꺼진 채 남는 소프트락이다. 같은 파일의 `Move Interactor To Target` 은 정확히 이 문제를 의식해 차단 대상을 인스턴스 데이터에 기록하고 `ExitState` 에서 짝 해제하므로(`:313-322`, `:510-528`), 한 파일 안에서 안전망 수준이 갈린다. `Content/WorldObject/Gimmick/ST_CutsceneTrigger.uasset` 이 이 태스크를 실제로 참조한다.
- **제안**: `Move Interactor To Target` 과 동형으로 만든다 — 연출 대상을 명시(기믹의 상호작용 당사자만 토글, 비대상 피어는 노옵)하고, `EnterState` 에서 실제로 끈 PC/Pawn 을 `TWeakObjectPtr` 로 기록해 `ExitState` 가 그 기록만 근거로 되돌린다.
- **확신도**: 중간(현재 에셋 배선에서 소프트락 발현은 어렵지만, 다중 피어 오작동과 안전망 부재는 확정)

### 5. 🟡 `SetInteractingCharacter` 를 아무도 호출하지 않아 Interactor 계열 태스크 2개가 휴면 상태다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:61-71`, 읽는 쪽은 `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:399`·`:469`·`:562`·`:584`
- **범주**: 중복/복잡도
- **문제**: `AWxGimmick::SetInteractingCharacter` 는 저장소 전체에서 호출부가 0개다(`UFUNCTION` 도 아니라 BP 에서도 호출 불가). 반면 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:54-62` 와 README 는 "자식이 `OnInteracted` 에서 `SetInteractingCharacter` → `CommitGimmickState`" 를 규약으로 명시하는데 기믹 7종 중 지키는 것이 하나도 없다. 결과적으로 복제 프로퍼티 `InteractingCharacter` 는 항상 null 이고, `Move Interactor To Target` 은 진입 즉시 경고 로그 후 Succeeded(`:404`), `Play Interactor Montage` 는 조용히 Succeeded 로 빠진다. ST 에셋 8개 중 두 태스크를 참조하는 것도 없어(`PlaySound` 도 마찬가지) 현재 발현은 없지만, 디자이너가 오늘 이 노드를 배치하면 원인을 알기 어렵게 무동작한다.
- **제안**: 배선 계획이 살아 있으면 자식마다 반복하지 말고 `AWxGimmick` 이 상호작용 진입점을 감싸 한 자리에서 당사자를 기록하도록 잇는다. 계획이 없으면 두 태스크·`InteractingCharacter` 복제 필드·규약 주석을 함께 정리한다. 되살릴 경우 2차 이슈도 같이 볼 것 — 이 태스크는 모든 피어에서 `ACharacter::SetActorLocation` 으로 직접 이동시키는데(`:484`) CMC 의 이동 복제·서버 보정을 끄지 않아 소유 클라 rubber-banding / 시뮬레이티드 프록시 떨림 가능성이 있다.
- **확신도**: 높음(호출부 부재는 확정. 의도된 미배선일 수 있음)

### 6. 🟡 스포너가 스폰한 Character 를 자신에게 영구 Attach 한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:175`
- **범주**: 설계/구조
- **문제**: `SpawnTarget` 이 `Spawned->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform)` 로 스폰체를 스포너에 붙이는데, 실제 스폰 대상은 CMC 로 돌아다니는 캐릭터이고 저장소 어디에도 detach 하는 코드가 없다. 루트가 attach 되어 있으면 `AActor::GatherCurrentMovement` 가 `ReplicatedMovement` 대신 `AttachmentReplication`(부모 상대 오프셋) 경로를 타므로 원격 클라에서 CMC 스무딩 경로를 벗어날 수 있고, 스포너를 옮기거나 회전시키면 AI 가 딸려 간다. 수명 관리 목적이라면 이미 `Respawn`(`:48-52`)·`EndPlay`(`:128-131`)가 `SpawnedActor` 약참조로 명시 Destroy 하고 있어 attach 는 불필요하다.
- **제안**: attach 를 제거하거나(권장), 아웃라이너 그룹핑이 목적이면 에디터 전용으로 한정한다. 유지해야 한다면 `OnSpawnedBy` 또는 빙의 직후 detach 를 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — detach 부재와 대상이 Character 라는 점은 확정)

### 7. 🟡 스포너 프리뷰 갱신이 에디터 로드마다 액터 라벨을 덮어쓴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:273-282` (호출 경로는 `:198-222` 의 `PostRegisterAllComponents`)
- **범주**: 설계/구조
- **문제**: `UpdateEditorPreviewFromSpawnableClass` 끝에서 `SetActorLabel(SpawnableActorClass 이름)` 을 무조건 호출하는데, 이 함수는 `PostEditChangeProperty`(클래스 변경 시 — 의도된 경로)뿐 아니라 `PostRegisterAllComponents`(`:198`)에서도 불린다. 즉 **에디터에서 맵/셀을 열 때마다** 라벨이 스폰 클래스 이름으로 되돌아간다. 디자이너가 스포너를 `Boss_Room_Guard_01` 처럼 이름 지어도 다음 로드에 사라지며, `AActor::SetActorLabel` 은 값이 다를 때 `Modify()` 를 태우므로 아무것도 편집하지 않아도 패키지가 dirty 로 표시되어 불필요한 소스 컨트롤 diff 를 만든다. 같은 클래스 스포너가 여럿이면 라벨이 전부 같아져 아웃라이너에서 구분도 되지 않는다.
- **제안**: 라벨 세팅을 `PostEditChangeProperty` 경로(클래스가 실제로 바뀐 순간)로 한정하거나, 라벨이 기본값(클래스명 계열)일 때만 갱신하고 사용자가 지정한 라벨은 보존한다.
- **확신도**: 중간(라벨을 항상 클래스에 동기화하려는 의도일 수 있으나, 로드마다 dirty 를 만드는 부작용은 확정)

### 8. 🟢 삭제된 조건 노드·존재하지 않는 함수를 가리키는 스테일 주석과 죽은 include 가 남아 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:8`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:51`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:56`
- **범주**: 중복/복잡도
- **문제**: (a) `FWxStateTreeCondition_GimmickStateIs` 가 제거되면서 `WxGimmickStateTreeNodes.h` 에는 조건 struct 가 하나도 남지 않았는데 `#include "StateTreeConditionBase.h"`(`:8`)는 그대로다. (b) `WxGimmick.h:51` 은 여전히 "ST 조건 'Wx Gimmick State Is' 등이 읽는다"고 적어 존재하지 않는 노드를 가리킨다(`README.md:25` 도 동일). (c) `WxGimmick.h:56` 은 "자식이 `HandleInteracted` 에서 호출한다"고 하는데 `HandleInteracted` 라는 함수는 현재 코드에 없다(실제 오버라이드 지점은 `OnInteracted`). 이 헤더 주석들이 신규 기믹 작성의 정본이라 오해 비용이 있다.
- **제안**: 죽은 include 를 지우고 두 주석을 현재 API 로 정정한다(발견 5 를 정리한다면 `:56` 은 함께 사라진다).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCutsceneTrigger.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCheckPoint.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxDoor.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCheckPoint.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxTreasureChest.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxTreasureChest.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxAlarmConsole.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxAlarmConsole.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxSpawnConsole.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxSpawnConsole.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` — 계약 교차 확인용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` 도 함께 읽음
- **직전 리뷰(커밋 `c42b5fec`) 지적 재확인**: 전수 액터 순회 스캔 🟡 는 **해소**됐다 — `ScanAndPush` 가 `OverlapMultiByObjectType` 반경 구로 후보를 모은다(`WxInteractionScannerComponent.cpp:152-188`). 폰 부재 조기 리턴 시 하이라이트 잔상 🟢 도 **해소**(`:128-135` 에서 `UpdateInRange({})`), `LogTemp` 🟢 도 **해소**(`WxWorldModule.h:8` 의 `LogWxWorld` 로 교체), `Component Move` 의 매 틱 `GetArchetype()` 🟢 도 **해소**(`EnterState` 가 `TargetLocation` 을 캐시, `WxGimmickStateTreeNodes.cpp:170`·`:199`). 나머지 6건은 현재 코드에서 재확인해 그대로 실었고, 발견 7 은 이번에 새로 잡혔다.
- **규칙 점검(위반 없음)**: 소스 29파일 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.` 확인 · `WxWorld.Build.cs` 의 Wx 의존은 `WxCore` 뿐(`WxTreasureChest.h:43` 의 `/Script/WxInventory.WxRewardTableRow` 는 meta 문자열이라 빌드 의존 아님) · `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll` 1곳으로 BP Function Library 예외에 해당 · `Wx` prefix 전수 준수 · 델리게이트 바인딩(`AddDynamic`/`AddUObject` 등)이 0건이라 `Handle` prefix 대상 없음 · 람다는 `WxInteractionScannerComponent.cpp:191` 의 정렬 술어 1개로 캡처가 필요한 필수 용례 · `Super::` 미호출 override 는 전부 순수 가상 인터페이스 함수(`IWxInteractable`/`IWxSavable`) 또는 엔진 ST 노드 훅이라 호출할 부모 구현이 없음
- **미검토 / 한계**: (1) `Content/WorldObject/Gimmick/ST_*.uasset` 8개는 참조 태스크 이름만 대조했고 상태/전이 구조·완료 판정·노드별 파라미터 값(특히 발견 1 의 모닥불 `bPlayOnRestore` 현재값, 발견 3 의 상태별 `Prompt` 실제 사용 여부)은 열어보지 않았다. (2) 기믹 BP 서브클래스의 디폴트값(할당된 ST 에셋, 프롬프트 문구)은 확인하지 않았다. (3) 멀티플레이 실기 검증은 하지 않았고 발견 2·4·5·6 의 네트워크 증상은 코드/엔진 경로 추론이다.

---
*문서 기준 커밋 `00b2e3f4` · 리뷰일 2026-07-26 · 소스 29파일 — `/module-review`로 갱신*
