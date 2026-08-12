# WxWorld — 코드 리뷰

> 기믹의 세 책임(상호작용·영속·ST 구동)을 컴포넌트 하나로 접고 상태 수렴을 「권위 폴링 → 복제 → 클라 추종 전이」 단일 경로로 정리한 구조가 견고하며, 널 가드·권위 가드·초기진입/라이브 분기·차단 해제 짝맞춤이 성실하다 — 남은 문제는 에디터 데이터 부여 경로 하나와, 연출 태스크 몇 개의 정리·대상 고정 누락에 몰려 있다. 이번 리뷰는 기믹 컴포넌트·스캐너·스포너와 위험도가 높은 ST 태스크(Move/Montage/SpawnActor/SpawnNiagara/PlayLevelSequence/Spline/Wait·TriggerSpawners) 의 cpp 를 정독하고, 판정 근거로 UE 5.8 엔진의 `UStateTreeComponent`·`FStateTreeExecutionContext`·`AActor` 프로퍼티 플래그·에디터 복제/붙여넣기 경로를 라인 단위로 대조했으며, 소비처(`WxAbility_Interact`·`WxViewModel_InteractionList`)와 `WxSave` 의 키 사용도 함께 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 에디터에서 스포너를 복제하면 `SaveId` 가 원본과 겹쳐, 세이브 레코드를 통째로 공유한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:192-205`(부여 훅 2종), `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:209-233`(이미 오버라이드된 재등록 훅)
- **범주**: 버그/정확성
- **문제**: `SaveId` 를 `PostActorCreated`/`PostDuplicate` 두 훅에서만 심는데, 에디터의 액터 복제(Ctrl+W·Alt-드래그)와 붙여넣기는 `StaticDuplicateObject` 가 아니라 **텍스트(T3D) 복사-붙여넣기 경로**다(엔진 `UUnrealEdEngine::DuplicateActors` 가 "copy-paste into the destination level" 로 구현). 그 경로의 순서는 `SpawnActor`(→ 새 `ActorGuid` 발급 → `PostActorCreated` 가 그 값을 `SaveId` 에 기록) → `PreEditChange(nullptr)`(컴포넌트 언레지스터) → `ImportObjectProperties`(**원본 T3D 로 `SaveId` 를 덮어씀**) → `PostEditImport` → `PostEditChange`(컴포넌트 재등록) 이라, `PostDuplicate` 는 애초에 불리지 않고 교정 기회도 없다. `ActorGuid` 는 엔진에서 `TextExportTransient, NonPIEDuplicateTransient` 로 선언돼 T3D 에 실리지 않으므로 새 값을 유지한다 — 즉 두 스포너의 `ActorGuid` 는 다른데 `SaveId` 만 같아진다.
  결과가 가볍지 않다. `WxSave` 는 `SaveId` 를 키로 레코드를 잡으므로(`Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:293`, `:355`) 두 스포너가 한 레코드를 공유한다 — 캡처는 서로 덮어쓰고, 복원은 **양쪽에 같은 `Transform` 을 적용해**(`:362`) 한 스포너가 다른 스포너 자리로 순간이동하며 `bIsKilled` 도 함께 물려받는다. 명시 세이브 때만이 아니라 WP 셀 스트리밍 아웃/인마다(`LevelRemovedFromWorld` 캡처 → `LevelAddedToWorld` 복원) 반복된다.
  같은 훅에 딸린 두 번째 구멍: `PostActorCreated`(`:192-197`)는 에디터 월드 여부를 가리지 않아 에디터 빌드(PIE 포함)에서 **런타임 스폰된** 스포너에도 그 세션 한정 GUID 를 심는다. "런타임 스폰된 것은 저장/복원에서 제외된다"(README) 는 계약이 에디터 빌드에서만 깨지고, 패키지 빌드(훅이 `WITH_EDITOR`)와 동작이 갈린다.
- **제안**: 같은 모듈의 기믹 컴포넌트가 이미 쓰는 compare-and-fix 로 통일한다(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:61-85`) — 이미 오버라이드돼 있고 게임 월드를 걸러내는 `PostRegisterAllComponents`(`:209-233`)에서 `SaveId != GetActorGuid()` 면 `Modify()` 후 갱신한다. 붙여넣기 경로의 마지막 `PostEditChange` 가 컴포넌트를 재등록하므로 이 훅은 import **뒤에** 다시 돌아 교정이 성립하며(기믹 컴포넌트가 지금 정상 동작하는 이유가 이것이다), 이미 어긋난 기존 배치도 맵을 열 때 함께 흡수된다. 교정 후 `PostActorCreated`/`PostDuplicate` 는 제거할 수 있다.
- **확신도**: 높음

### 2. 🟡 `Spawn Niagara` 에 정리 경로가 없어 상태를 떠나도 FX 를 끌 수 없고, 미부착 경로는 오너보다 오래 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_SpawnNiagara.cpp:16-48`(`ExitState` 부재), 계약 서술은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_SpawnNiagara.h:46-52`
- **범주**: 버그/정확성
- **문제**: 이 태스크는 `EnterState` 만 있고 `ExitState` 가 없다. 헤더는 "루프 Niagara 를 지정하면 **상태에 묶인** 지속 FX" 라고 계약을 서술하지만, 상태를 떠날 때 `SpawnedComponent` 를 정지·파괴하는 자리가 없어 켤 수는 있어도 끌 수가 없다 — 상태 A 에서 켠 레이저·불꽃이 B 로 가도 계속 돈다. `bAutoDestroy` 는 시스템이 **완료될 때** 파괴한다는 뜻이라 루프 시스템에서는 영원히 발동하지 않는다. 게다가 `AttachComponent` 를 비운 경로(`:43`)는 `SpawnSystemAtLocation` 으로 월드 소유 컴포넌트를 만들어 오너에 붙지 않으므로, 기믹 액터가 파괴되거나 WP 셀이 언로드돼도 FX 가 월드에 남는다. 형제 태스크는 둘 다 자기 산출물을 회수한다 — `WxStateTreeTask_PlayLevelSequence.cpp:68-73`, `WxStateTreeTask_SpawnActor.cpp:80-103`.
- **제안**: `ExitState` 를 추가해 `SpawnedComponent` 가 유효하면 `Deactivate()`(즉시 제거가 필요하면 `DestroyComponent()`) 후 핸들을 비운다. 체크포인트 모닥불처럼 이탈 후에도 남겨야 하는 용례가 있으므로, `SpawnActor` 의 `bDestroyOnExit` 와 같은 인스턴스 데이터 플래그로 두 동작을 노출하는 편이 낫다.
- **확신도**: 중간(정리 경로 부재는 코드로 확정. 모닥불 용례를 위해 의도적으로 뺐을 수 있으나, 그렇다면 헤더의 "상태에 묶인" 서술과 미부착 경로의 잔존이 함께 정리돼야 한다)

### 3. 🟡 Interactor 계열 태스크가 대상 캐릭터를 매 틱 라이브 재조회하고, 대상이 사라지면 상태를 `Failed` 로 떨어뜨린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp:103-109`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayInteractorMontage.cpp:47-53`, 값의 유일한 대입은 `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:133`
- **범주**: 버그/정확성
- **문제**: 두 태스크의 `Tick` 은 매 프레임 `Gimmick->GetInteractingCharacter()` 를 다시 읽는데, 이 값은 권위 측이 상호작용마다 덮어쓰고 복제하는 라이브 멤버이며 어디서도 비워지지 않는다. 정작 같은 파일의 `ExitState`(`MoveInteractorToTarget.cpp:146-164`)는 정확히 이 이유로 **차단 대상만은** 인스턴스 데이터에 스냅샷해 두고 있다(`:22`, `:59`, `:64`) — 이동·판정 대상만 그 보호를 못 받는다.
  - 이동 중 다른 플레이어가 같은 기믹의 (아직 켜져 있는) 다른 영역을 누르면, `MoveSpeed`/`TurnSpeed` 는 A 기준으로 산출된 채 B 를 `SetActorLocation` 으로 끌고 가고, `PlayInteractorMontage` 는 재생한 적 없는 B 의 몽타주를 폴링해 `Montage_IsPlaying == false` 로 즉시 `Succeeded` 를 반환해 연출이 조기 종료된다.
  - 대상이 이동 중 파괴·언포제스·릴러번시 이탈로 사라지면 `Tick` 이 `Failed` 를 돌려 **상태 자체가 실패**하므로, 실패 전이가 없는 기믹은 그 자리에서 멈춘다(클라에서만 사라진 경우 서버와 어긋났다가 추종 전이로 되돌아오는 왕복도 생긴다).
- **제안**: `BlockedController` 와 동형으로 `EnterState` 에서 대상 캐릭터를 `TWeakObjectPtr` 인스턴스 데이터에 기록하고 `Tick`·완료 판정은 그 기록만 근거로 쓴다. 기록이 죽었으면 `Failed` 대신 `Succeeded` 로 빠져 상태가 갇히지 않게 한다(진입부의 "대상 없으면 곧바로 완료" 정책과 같은 방향).
- **확신도**: 중간(재조회·미소거·`Failed` 반환은 코드로 확정. 다중 영역 동시 상호작용 발현 여부는 ST 에셋 배선에 달려 있다)

### 4. 🟡 `Move Interactor To Target` 의 스크립트 이동이 CMC 와 권위를 다투고, sweep·타임아웃이 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp:43-47`(진입 시 1회 정지), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp:116-134`(틱 이동·완료 판정)
- **범주**: 설계/구조
- **문제**: 진입 시 `StopMovementImmediately()` 로 속도만 한 번 0으로 만들 뿐 CMC 를 멈추지는 않는다. 슬라이드가 도는 동안에도 중력·`PhysWalking`·스텝업·클라 예측 보정이 매 프레임 그대로 돌고, 태스크는 그 뒤(액터 간 틱 순서는 보장되지 않는다)에 `SetActorLocation` 으로 다시 밀어 넣는다. 셋이 따라온다. (a) 앵커가 지면에서 떨어져 있으면(플랫폼·계단 위 상호작용 지점) 낙하와 슬라이드가 경합해 `MoveSpeed` 가 낙하 속도보다 느린 축에서 수렴하지 못한다. (b) `SetActorLocation` 이 sweep 없이 호출되어(`:120`, 즉시 스냅 경로 `:79` 도 동일) 얇은 지오메트리를 관통한다. (c) 완료 판정이 위치 `Equals` 하나뿐이라(`:131`) 되밀림이 생기면 상태가 무한정 길어진다 — 이 태스크는 상태 완료의 트리거이므로 기믹 전체가 그 자리에 머문다.
- **제안**: 진입 시 `DisableMovement()`(또는 `SetMovementMode(MOVE_None)`)로 CMC 를 끄고, 이미 차단 해제 기록을 들고 있는 `ExitState` 에서 원복해 슬라이드 구간의 이동 권위를 태스크 하나로 모은다. 최소 조치로도 sweep 을 켜 관통을 막고 `Duration` 기준 타임아웃(초과 시 목표로 스냅 후 `Succeeded`)을 둔다.
- **확신도**: 중간

### 5. 🟡 `Enable Player Input` 이 상호작용 당사자가 아니라 「이 머신의 첫 로컬 플레이어」를 막는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp:29-35`, 한계 서술은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_EnablePlayerInput.h:43-44`
- **범주**: 설계/구조
- **문제**: 기믹 ST 는 모든 피어에서 각자 도는데 대상 선택이 `GEngine->GetFirstLocalPlayerController(World)` 다. 리슨 호스트에서 원격 클라가 기믹을 발동하면 **연출과 무관한 호스트 플레이어의 입력이 연출 내내 막히고**, 스플릿스크린 2P 이상은 반대로 아무도 막히지 않는다. 헤더가 이미 한계와 해법("오너 기믹의 `GetInteractingCharacter` 를 읽으면 된다")을 적어 둔 의식적 보류지만, 멀티플레이를 지원하는 이상 남겨 두면 그대로 버그로 드러난다.
- **제안**: 형제 노드 `MoveInteractorToTarget` 이 이미 쓰는 패턴을 그대로 따른다 — 오너 기믹의 복제된 `GetInteractingCharacter()` 로 대상을 좁히고 `IsLocallyControlled()` 로 게이팅한다. 해제 기록(`DisabledPawn`/`DisabledController`)은 지금 구조 그대로 쓰면 된다.
- **확신도**: 높음(동작은 코드로 확정. 우선순위는 멀티플레이 지원 범위에 대한 판단에 달림)

### 6. 🟡 `SpawnTarget()` 의 침묵 실패가 `Wait Spawners Killed` 를 영구 대기로 몰아넣는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:149-175`, 소비 짝은 `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:38-64`
- **범주**: 버그/정확성
- **문제**: `Respawn()` 은 `bIsKilled = false` 를 먼저 쓰고 `SpawnTarget()` 을 부르는데, `SpawnTarget()` 은 `SpawnableActorClass` 가 비어 있으면 로그 한 줄 없이 반환하고(`:151-154` — 경고는 인터페이스 미구현 케이스 `:156-160` 에만 있다) `SpawnActorDeferred` 실패(`:172-175`)도 침묵한다. 그러면 스포너는 "살아 있지만 인스턴스가 없는" 상태가 되고, 실제 배선돼 있는 `Trigger Spawners By Locator` → `Wait Spawners Killed` 짝(`Content/Quest/ST_Quest_Main1.uasset`)은 죽일 대상이 없는 채 `IsKilled()==false` 를 영원히 읽어 퀘스트 ST 가 그 상태에 갇힌다. `WaitSpawnersKilled` 가 빈 배열·빈 로케이터에는 굳이 경고를 남겨 침묵 대기를 막으려 한 것(`:21-33`)과 같은 성격의 구멍이 반대편에 남아 있다.
- **제안**: `SpawnTarget()` 이 클래스 미지정·스폰 실패에도 `LogWxWorld` 경고를 남기게 하고(인터페이스 미구현 케이스와 같은 수준), `bool` 을 돌려 `Respawn()` 이 실패를 진단 가능한 형태로 남기도록 한다. `bIsKilled` 리셋 자체는 유지하는 편이 낫다 — 실패 시 `true` 로 두면 Wait 가 즉시 통과해 오히려 조용히 건너뛴다.
- **확신도**: 중간

### 7. 🟢 `StartTreeAtSavedState` 가 엔진 `StartTree` 를 재조립하면서 재진입 가드를 가져오지 못했다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:290-341`
- **범주**: 성능/안전
- **문제**: 시작 상태를 넣기 위해 엔진 `UStateTreeComponent::StartTree` 를 복제해 왔는데, 원본이 컨텍스트 생성 전후에 두르는 재진입 체크와 `TGuardValue`(엔진 `StateTreeComponent.cpp` 의 `CurrentlyRunningExecContext`)가 빠졌다. **원본대로 옮겨오는 것은 불가능하다** — 그 멤버가 엔진 헤더에서 `private` 이라 파생 클래스가 읽지도 쓰지도 못한다. 지금 `RestartLogic` 호출부는 `OnSaveRestored`(`:161-169`) 하나뿐이고, `WxSave` 의 복원은 레벨 스트리밍/월드 초기화 콜백이라 ST 틱 밖이므로 잠재 위험에 머문다. 다만 훗날 ST 태스크나 전이 처리 안에서 복원·재시작을 부르는 경로가 생기면 같은 `InstanceData` 위에 두 번째 실행 컨텍스트가 만들어진다. 컨텍스트 요구사항 실패 시 엔진이 남기는 경고 로그도 함께 빠져 있다.
- **제안**: 자체 재진입 플래그를 `StartTreeAtSavedState` 진입부에 두어 중첩 호출을 로그 후 반환시키고, 이미 "엔진 업그레이드 시 확인 지점" 으로 표시된 헤더 주석(`Public/Gimmick/WxGimmickStateTreeComponent.h:167-171`)에 "ST 태스크의 Enter/ExitState 에서 이 컴포넌트의 Start/Stop/RestartLogic 을 부르지 말 것" 을 계약으로 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 현재 재진입 경로는 없고, 엔진이 멤버를 열어 주기 전에는 완전한 대안이 없다)

### 8. 🟢 타이머에 바인딩되는 `ScanAndPush` 가 `Handle` 접두사 규칙을 따르지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:44`(바인딩), 선언은 `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:102`
- **범주**: 규칙 위반 (코딩 규칙 4 — Delegate 에 바인딩되는 Callback 은 `Handle` prefix)
- **문제**: `SetTimer(..., &UWxInteractionScannerComponent::ScanAndPush, ...)` 로 `FTimerDelegate` 에 직접 바인딩된다. 모듈 안의 유일한 사례이며, 프로젝트의 다른 타이머 콜백은 규칙을 지킨다(예: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp` 의 `HandleMontagePollTick`).
- **제안**: 타이머 바인딩용 `HandleScanTimer()` 를 두고 그 안에서 `ScanAndPush()` 를 부른다 — 이 함수는 델리게이트 외에 직접 호출부(`:48`)도 있으므로 단순 개명보다 분리가 낫다.
- **확신도**: 높음

### 9. 🟢 `GetPrompts()` 가 죽은 약참조 항목을 건너뛰어 선택 인덱스와 어긋날 수 있다(주석은 반대로 서술)
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:76-91`(스킵은 `:82`), 소비처는 `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43-44`
- **범주**: 버그/정확성
- **문제**: 주석(`:85-86`)은 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다" 고 하지만, 자리를 채우는 것은 `IWxInteractable::Find` 가 실패한 경우뿐이고 **메시 자체가 이미 파괴된 항목은 `:82` 에서 통째로 건너뛴다**. `UpdateInRange` 내부 호출은 바로 앞(`:218-227`)에서 무효 항목을 떼어낸 뒤라 안전하지만, 이 함수는 뷰모델이 구독 직전 시드로 임의 시점에 부르는 공개 API 다 — 마지막 스캔 이후(최대 `ScanInterval`=0.1초) 대상이 파괴됐으면 `GetPrompts().Num() < InRangeMeshes.Num()` 이 되어 같은 자리에서 함께 읽는 `GetSelectedIndex()` 와 줄이 어긋난다. 실제 선택 대상은 맞고 표시만 다른 줄이 강조되는 형태라 원인을 찾기 어렵다.
- **제안**: 스킵 대신 빈 텍스트로 자리를 채워 주석이 서술한 계약과 코드를 맞춘다(또는 주석을 코드에 맞추고, 시드 시점에 스캔을 한 번 강제해 두 값이 같은 스냅샷에서 나오게 한다).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_SpawnActor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 기믹 ST 태스크 전 파일(`WxStateTreeTask_ComponentMove`·`ComponentSplineMove`·`EnableInteraction`·`PlayAnimation`·`PlaySound`·`ApplyGameplayEffectToInteractor`·`TriggerSpawners`·`RespawnSpawners` 의 h/cpp), `Plugins/WxWorld/Source/WxWorld/{Public,Private}/System/*`, `.../Spawnable/WxSpawnable.*`, `.../WxWorldModule.*` — 계약 교차 확인용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`·`WxSavable.h`·`Private/WxInteractable.cpp`, 소비처 확인용으로 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.{h,cpp}`·`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, 영속 경로 확인용으로 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp` 도 함께 읽음
- **코딩 규칙 스캔 결과(통과)**: Copyright 첫 줄 46/46 · `Wx` prefix 전수 준수 · Wx 의존은 `WxCore` 뿐이고 다른 Wx 플러그인 헤더를 포함하는 곳 없음(`WxWorld.Build.cs:24`) · `BlueprintCallable` 은 `Public/System/WxSpawnerLibrary.h:29` 한 곳(BP Function Library) · `FORCEINLINE` 없음 · 헤더 인라인 정의는 `GetInstanceDataType()` 뿐이고 각 노드 헤더 상단이 규칙 6 예외임을 명시(엔진 ST 노드 관례와 동일하므로 조치보다 CLAUDE.md 규칙 6 에 예외를 명문화하는 편이 낫다) · 람다는 스캐너의 거리순 정렬 술어(`WxInteractionScannerComponent.cpp:202`) 하나로 캡처가 필요해 대체가 마땅치 않음 · 위반은 발견 8 하나
- **발견으로 올리지 않은 관찰**: (a) 직전 리뷰의 🔴(「`OnRep_StateTag` 가 못 따라온 피어를 오판해 트리를 재시작」)는 현재 코드에서 해소됐다 — OnRep 은 틱만 깨우고(`WxGimmickStateTreeComponent.cpp:251-269`), 대조는 트리 틱 뒤 `FollowAuthorityState` 가 하며(`:388-413`), 교정도 재시작이 아니라 `RequestTransition(Critical)` 이라(`:415-436`) 대기 이벤트와 인스턴스 데이터가 보존된다. 잠든 트리의 깨우기도 엔진이 `SendEvent`/`RequestTransition` 에서 실행 확장을 통해 예약하는 것을 확인했다. (b) `Component Move` 가 기준 포즈를 컴포넌트 **아키타입**에서 읽는 것(`WxStateTreeTask_ComponentMove.cpp:27-28`)은 헤더가 계약으로 명시한 의도된 설계라 뺐다(대신 레벨 배치 시 컴포넌트 개별 오프셋 금지를 관례로 둘 것). (c) 스캔 반경이 스캐너(`Public/Interaction/WxInteractionScannerComponent.h:82`)와 서버 검증(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h:39`)에 각각 150cm 로 이중 소유된 것은 변조 방지 근거가 양쪽 주석에 명시돼 있어 제외했다 — 다만 어긋나면 프롬프트가 뜨는데 서버가 조용히 거부하므로, 거부 시 개발 빌드 로그 한 줄은 값이 있다. (d) `Trigger Spawners`(소프트 포인터·바인딩형)와 `Trigger Spawners By Locator`(리터럴형)의 `EnterState` 본문이 거의 같지만 대상 지정 방식이 달라 서로를 대체하지 못하므로 중복으로 올리지 않았다(현재 에셋에 실린 것은 By Locator 뿐). (e) `WaitSpawnersKilled` 의 매 틱 `SyncFind` 와 `ScanAndPush` 의 0.1초 주기 전 오브젝트 채널 오버랩 + `Examined.Contains` 선형 탐색은 둘 다 헤더가 의도된 트레이드오프로 명시했고 규모가 작아 제외했다.
- **미검토 / 한계**: (1) `Content/WorldObject/Gimmick/ST_*.uasset`·`Content/Quest/ST_Quest_Main1.uasset` 의 상태·전이·태스크 파라미터 배선은 읽지 않았다 — 발견 2·3·6 의 **발현 여부**는 그 배선에 달려 있다(존재 자체는 코드로 확정). (2) 멀티플레이·에디터 실기 검증 없음 — 발견 1 의 복제 경로는 엔진 `UUnrealEdEngine::DuplicateActors`·`ULevelFactory::FactoryCreateText`·`AActor` 프로퍼티 플래그 기반 정적 분석이고, 발견 4 의 CMC 경합도 같다. (3) 에디터 전용 프리뷰 경로(`AWxSpawner::UpdateEditorPreviewFromSpawnableClass` 의 바운드 계산, `UWxWorldDeveloperSettings` 아이콘 매핑)는 읽었으나 에디터에서 확인하지 않았다. (4) 스캐너가 Experience 주입으로 서버·클라 양쪽에 각각 붙는지(붙으면 클라에 복제본과 로컬본이 겹칠 수 있다)는 주입 설정 에셋을 열지 않아 확인하지 못했다.

---
*문서 기준 커밋 `98c35dfd` · 리뷰일 2026-08-12 · 소스 46파일 — `/module-review`로 갱신*
