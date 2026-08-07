# WxWorld — 코드 리뷰

> 기믹의 세 책임(상호작용·영속·ST 구동)을 컴포넌트 하나로 접고 동작은 전부 ST 에셋이 author 하는 구조가 일관되며, 널 가드·권위 가드·초기진입/라이브 분기·차단 해제 짝맞춤이 전반적으로 성실하다 — 남은 문제는 대부분 「멀티플레이 수렴 경로」와 「에디터 데이터 부여 경로」에 몰려 있다. 이번 리뷰는 C++ 18파일 전부를 열고 기믹 컴포넌트·기믹 ST 노드 라이브러리·스캐너·스포너·스포너 ST 노드의 cpp 를 정독했으며, 판정 근거로 엔진(UE 5.8) `UStateTreeComponent`·`FStateTreeExecutionContext`·`FStateTreeInstanceStorage`·`AActor` 구현을 라인 단위로 대조하고, 발현 여부 확인용으로 `.claude/asset_dump/` 의 기믹 BP 4종·ST 4종 덤프를 함께 읽었다. 직전 리뷰(`1e9b745c`) 이후 이 모듈의 코드 변경은 `c50d3d32`(NPC 상호작용 토글, WxWorld 소스 무변경) 뿐이라 지적은 전부 재확인 후 이월했고, 확인된 발현 범위에 맞춰 심각도만 조정했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 `OnRep_StateTag` 가 아직 처리되지 않은 대기 이벤트를 "어긋난 피어"로 오판해, 클라에서 트리거형 연출이 통째로 스킵된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:219-234`(OnRep), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:236-246`(Multicast)
- **범주**: 버그/정확성
- **문제**: `Multicast_Interact_Implementation` 의 `SendStateTreeEvent`(`:245`)는 이벤트를 **큐에 넣기만** 한다 — 엔진 `StateTreeComponent.cpp:591-592` → `FStateTreeMinimalExecutionContext::SendEvent`(`StateTreeExecutionContext.cpp:932-935`)가 큐에 append 하고 틱만 예약하며, 실제 소비는 다음 트리 틱의 전이 처리다. 반면 `OnRep_StateTag` 는 그 자리에서 `GetActiveStateTag() == StateTag` 를 비교하므로(`:228`), 멀티캐스트 RPC 와 `StateTag` 프로퍼티가 같은 네트 업데이트로 도착하면(둘 다 `TickDispatch` 에서 처리되어 사이에 트리 틱이 없다) 판정 시점의 로컬 트리는 아직 **이전 상태**다 → 불일치로 보고 `RestartLogic()`(`:233`)을 탄다. 재시작은 `FStateTreeExecutionContext::Start` 안에서 `InstanceData.Reset()` 을 **무조건** 수행하고(엔진 `StateTreeExecutionContext.cpp:1493`), 그 `Reset` 이 큐를 소유한 기본 경로에서 `EventQueue->Reset()` 으로 대기 큐를 비운다(엔진 `StateTreeInstanceData.cpp:776-779`). 즉 **큐에 있던 Interact 이벤트가 폐기되고**, 새 진입은 `SourceStateID` 가 무효인 초기 진입이 되어 `IsInitialEntry` 로 갈리는 태스크가 전부 스킵된다 — PlaySound(`WxGimmickStateTreeNodes.cpp:835`), PlayLevelSequence(`:757`), PlayInteractorMontage(`:666`), MoveInteractorToTarget(`:501`), ApplyGameplayEffectToInteractor(`:128`), TriggerSpawners(`:930`), TriggerSpawnersByLocator(`WxSpawnerStateTreeNodes.cpp:82-86`). 도착 순서가 반대여도(프로퍼티가 먼저) 결과는 같다 — OnRep 이 먼저 초기 진입으로 재시작해 연출은 역시 스킵된다. 리슨호스트 로컬에서는 정상으로 보여 PIE 단일 플레이 테스트로는 드러나지 않는다. 현재 배치된 기믹 ST 4종(Door·Elevator·CheckPoint·TreasureChest)에는 클라 가시 트리거형 태스크가 없어(`ApplyGameplayEffectToInteractor`·`RespawnSpawners` 는 권위 게이트, `ComponentMove`·`EnableInteraction`·`PlayAnimation`·`SpawnNiagara` 는 진입 경로 무관) 아직 눈에 띄지 않을 뿐, 기믹에 `PlaySound`·`PlayLevelSequence` 를 다는 순간 바로 드러난다.
- **제안**: OnRep 시점에 즉시 판정하지 않는다 — (a) 대기 중인 `StateTree.Interact` 이벤트가 있으면 이번 판정을 건너뛰거나, (b) 판정을 다음 틱 말미(`RefreshStateTag` 와 같은 자리)로 미뤄 로컬 트리가 큐를 소화한 뒤 비교하거나, (c) 복제 대상에 "직전 전이 시퀀스 번호"를 추가해 「아직 못 따라온 것」과 「진짜 어긋난 것」을 구분한다. 검증은 PIE 2-클라이언트에서 `PlaySound` 를 단 기믹을 리모트 클라 화면으로 확인하면 즉시 갈린다.
- **확신도**: 중간(엔진 큐잉·`InstanceData.Reset()`·이벤트 큐 클리어는 라인 단위로 확인. 상시 발현인지 간헐인지는 서버 프레임 내 「어빌리티 실행 vs 기믹 컴포넌트 틱」 순서에 달려 있어 미실측 — 컴포넌트 틱이 어빌리티보다 먼저 돌면 `StateTag` 변경이 한 프레임 밀려 정상 경로로 수렴한다)

### 2. 🟡 `AWxSpawner` 의 SaveId 부여 훅이 에디터 복제 경로를 놓쳐, 복제된 스포너끼리 세이브 키가 충돌한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:192-205`
- **범주**: 버그/정확성
- **문제**: `SaveId` 를 `PostActorCreated`/`PostDuplicate` 두 훅에서만 심는다. 그런데 에디터의 액터 복제(Ctrl+D·Alt-드래그·복사/붙여넣기)는 `StaticDuplicateObject` 가 아니라 텍스트 export/import 경로다 — 새 액터를 `SpawnActor` 로 만든 뒤(`PostActorCreated` 통과) 원본 T3D 를 import 로 덮어쓰므로, 플래그 없는 평범한 `UPROPERTY` 인 `SaveId`(`WxSpawner.h:82-84`)는 **원본 값으로 되돌아가고** `PostDuplicate` 는 불리지 않아 교정 기회도 없다. 반면 `ActorGuid` 는 엔진에서 `TextExportTransient`·`NonPIEDuplicateTransient` 로 선언돼(엔진 `Actor.h:1096`) T3D 에 실리지 않고 스폰 시 새로 발급된 값을 유지한다. 결과적으로 복제한 스포너 둘이 같은 `SaveId` 로 한 슬롯 레코드를 공유하므로, 한쪽 보스를 잡으면 다른 쪽도 처치 상태로 복원된다(`bIsKilled` 는 `SaveGame`, `WxSpawner.h:78-80`). 같은 이유로 이 코드 이전에 배치된 스포너는 두 훅이 로드 시 불리지 않아 `SaveId` 가 무효인 채 남고, `IWxSavable::GetSaveId` 규약상 저장/복원에서 **아무 신호 없이** 제외된다(`Plugins/WxCore/Source/WxCore/Public/WxSavable.h:35`). 같은 모듈의 기믹 컴포넌트는 이 문제를 이미 다르게 풀었다 — `OnRegister` 에서 매 등록마다 오너 `ActorGuid` 와 대조해 신규 배치·복제·기존 마이그레이션을 한 경로로 처리하고, 그 주석(`WxGimmickStateTreeComponent.cpp:61-62`)이 액터 단위 사전 훅을 쓸 수 없는 이유까지 적어 두었다.
- **제안**: `WxGimmickStateTreeComponent::OnRegister`(`:49-73`)와 동일한 패턴으로 통일한다 — 스포너에 이미 오버라이드돼 있는 `PostRegisterAllComponents`(`WxSpawner.cpp:209`, 에디터 월드 한정)에서 `GetActorGuid()` 와 비교해 다르면 `Modify()` 후 갱신(같으면 노옵). 기존 두 훅은 그때 제거할 수 있다. 기믹이 쓰기 전에 `Modify()` 를 부르는데 스포너의 두 훅은 부르지 않는 점도 함께 맞춘다.
- **확신도**: 높음(값 복사와 `ActorGuid` 의 텍스트 export 제외는 엔진 프로퍼티 플래그로 확정. 에디터 복제가 `PostDuplicate` 를 타지 않는다는 점만 경로 추론이라 실측 권장)

### 3. 🟡 스폰에 실패한 `Respawn()` 이 `bIsKilled` 를 리셋해, `WaitSpawnersKilled` 가 조용히 영구 대기한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:65-72`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:149-160`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:194-201`
- **범주**: 버그/정확성
- **문제**: `Respawn()` 은 `bIsKilled = false`(`:71`)를 먼저 쓰고 `SpawnTarget()` 을 부른다. `SpawnTarget()` 은 `SpawnableActorClass` 가 비어 있으면 로그 한 줄 없이 반환하고(`:151-154` — 경고는 인터페이스 미구현 케이스인 `:156-160` 에만 있다), `SpawnActorDeferred` 실패(`:172-175`)도 마찬가지로 침묵한다. 그러면 스포너는 "살아 있지만 인스턴스가 없는" 상태가 되고, `TriggerSpawnersByLocator` → `WaitSpawnersKilled` 짝(헤더가 "같은 상태에서 짝으로 쓴다"고 명시한 조합, `WxSpawnerStateTreeNodes.h:88`)은 죽일 대상이 없는 채 `IsKilled()==false` 를 영원히 읽어 ST 가 그 상태에 갇힌다. `WaitSpawnersKilled` 가 빈 배열·빈 로케이터에는 굳이 경고를 남겨 침묵 대기를 막으려 한 것(`:160-172`)과 같은 성격의 구멍이 반대편에 남아 있다.
- **제안**: `SpawnTarget()` 이 클래스 미지정·`SpawnActorDeferred` 실패에도 `LogWxWorld` 경고를 남기게 하고(현재 인터페이스 미구현 케이스와 같은 수준), 성공 여부를 `bool` 로 돌려 `Respawn()` 이 실패를 진단 가능한 형태로 남기도록 한다. `bIsKilled` 리셋 자체는 유지하는 편이 낫다 — 실패 시 `true` 로 두면 Wait 가 즉시 통과해 오히려 조용히 건너뛴다.
- **확신도**: 중간

### 4. 🟡 Interactor 계열 태스크가 대상 캐릭터를 매 틱 라이브 재조회해, 도중에 당사자가 바뀌거나 죽으면 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:574-584`(Move Tick), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:690-709`(Montage Tick)
- **범주**: 버그/정확성
- **문제**: 두 태스크의 `Tick` 은 매 프레임 `Gimmick->GetInteractingCharacter()` 를 다시 읽는다(`:580`, `:695`). 그런데 `InteractingCharacter` 는 권위 측이 상호작용마다 멀티캐스트로 덮어쓰는 라이브 멤버다(`WxGimmickStateTreeComponent.cpp:239`) — 같은 파일의 `ExitState` 주석(`:624-626`)이 정확히 이 이유로 **차단 대상만은** 인스턴스 데이터에 스냅샷해 두는데(`:534`, `:539`), 정작 이동/판정 대상은 스냅샷하지 않는다. 이동 중 다른 플레이어가 같은 기믹의 (아직 켜져 있는) 다른 영역을 누르면, `MoveSpeed`/`TurnSpeed` 는 A 기준으로 산출된 채 B 를 `SetActorLocation` 으로 끌고 가고, `PlayInteractorMontage` 는 재생한 적 없는 B 의 몽타주를 폴링해 `Montage_IsPlaying == false` 로 즉시 Succeeded 를 반환해 연출이 조기 종료된다(`:709`). 또한 당사자가 이동 중 파괴·언포제스되면 `Tick` 이 `Failed` 를 돌려(`:583`) 상태 자체가 실패하므로, 실패 전이가 없는 기믹은 그 자리에서 트리가 멈춘다.
- **제안**: `BlockedController` 와 동형으로 `EnterState` 에서 대상 캐릭터를 `TWeakObjectPtr` 인스턴스 데이터에 기록하고, `Tick`/완료 판정은 그 기록만 근거로 쓴다(기록이 죽었으면 `Failed` 대신 `Succeeded` 로 빠져 상태가 갇히지 않게).
- **확신도**: 중간(재조회와 `Failed` 반환은 코드로 확정. 현재 기믹 ST 4종은 두 태스크를 쓰지 않아 발현 사례가 없다 — 다중 영역 기믹에 처음 얹을 때 대비 항목이다)

### 5. 🟡 `SpawnNiagara` 에 정리 경로가 없어, 상태를 떠나도 FX 가 그대로 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:873-905`, 선언은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:605-620`
- **범주**: 버그/정확성
- **문제**: 이 태스크는 `EnterState` 만 가지고 `ExitState` 가 없다. 헤더는 "루프 Niagara 를 지정하면 **상태에 묶인** 지속 FX"(`h:601`)라고 계약을 서술하지만, 상태를 떠나도 `SpawnedComponent` 를 정지·파괴하는 자리가 어디에도 없어 FX 는 계속 재생된다 — 즉 켤 수는 있어도 끌 수가 없다. 두 스폰 경로 모두 `bAutoDestroy=true` 지만(`cpp:896` 명시, `:900` 기본값) 이는 시스템이 **완료될 때** 파괴한다는 뜻이라 루프 시스템에는 영원히 발동하지 않는다. 게다가 `AttachComponent` 를 비운 경로는 `SpawnSystemAtLocation` 으로 월드 소유 컴포넌트를 만들어 오너에 붙지 않으므로, 기믹 액터가 파괴되거나 WP 셀이 언로드돼도 FX 가 월드에 남는다. 지금은 `ST_CheckPoint` 의 모닥불(종착 상태라 이탈이 없음)에서만 쓰여 발현하지 않을 뿐, 상태 A 에서 켜고 B 에서 꺼야 하는 첫 기믹에서 바로 드러난다. 같은 파일의 `PlayLevelSequence`(`:803-808`)·`SpawnActor`(`:1042-1065`)는 둘 다 `ExitState` 로 자기 산출물을 회수한다.
- **제안**: `ExitState` 를 추가해 `SpawnedComponent` 가 유효하면 `Deactivate()`(또는 `DestroyComponent()`) 후 핸들을 비운다. 지속 FX 를 이탈 후에도 남기고 싶은 경우가 있다면 `SpawnActor` 의 `bDestroyOnExit` 처럼 인스턴스 데이터 플래그로 노출한다.
- **확신도**: 중간(정리 경로 부재는 코드로 확정. "상태 이탈 시 남는 것이 의도"일 가능성은 있으나 헤더 서술과 형제 태스크 관례 양쪽과 어긋난다)

### 6. 🟡 진행 중인 작업을 다루는 Interactor 태스크 2종이 `bShouldStateChangeOnReselect` 를 선언하지 않아, 재선택에 연출이 처음부터 다시 돈다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:429-444`(MoveInteractorToTarget), `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:465-479`(PlayInteractorMontage)
- **범주**: 설계/구조
- **문제**: 두 태스크만 생성자가 아예 없어 엔진 기본값 `true` 를 그대로 쓴다. 그런데 같은 헤더가 그 정책을 이렇게 못박아 두었다 — "발동 순간에 한 번 일어나는 액션형은 true, 진행 중인 작업이 재선택에 끊기지 않아야 하는 것은 false"(`h:56-58`). 실제로 형제 태스크는 전부 false 를 선언한다: EnableInteraction(`cpp:66`), EnablePlayerInput(`cpp:211`), ComponentMove(`cpp:280`), ComponentSplineMove(`cpp:358`), PlayLevelSequence(`cpp:749`), SpawnActor(`cpp:974`), WaitSpawnersKilled(`WxSpawnerStateTreeNodes.cpp:153`). 재선택(Sustained)이 걸리면 `MoveInteractorToTarget` 은 `ExitState`(입력 차단 해제)→`EnterState`(재차단, `cpp:564` 에서 남은 거리를 전체 Duration 으로 다시 나눠 `MoveSpeed` 재산출)로 이동이 매번 느려지고, `PlayInteractorMontage` 는 몽타주를 처음부터 다시 튼다 — 재선택이 반복되면 끝나지 않는다. 헤더 자신이 "루트 재선택 thrash"(`h:61`)를 실재하는 위험으로 경고하는 만큼 가정만의 시나리오가 아니다.
- **제안**: 두 태스크에 생성자를 추가해 `bShouldStateChangeOnReselect = false` 를 선언한다. `PlayAnimation`(`h:357-371`)도 같은 모양이지만 헤더가 "진입 경로 무관 처음부터 재생"을 명시적 방침으로 적어 두었으므로, 유지한다면 규약 예외임을 생성자 주석으로 드러낸다.
- **확신도**: 중간(기본값과 형제 관례는 코드로 확정. 현재 ST 에셋에 두 태스크가 실려 있지 않아 발현 사례는 없다)

### 7. 🟡 `MoveInteractorToTarget` 의 스크립트 이동이 CMC·이동 복제와 권위를 다툰다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:519-522`(진입), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:591-596`(틱 이동)
- **범주**: 설계/구조
- **문제**: 진입 시 `StopMovementImmediately()` 로 속도만 1회 0으로 만들 뿐 CMC 를 멈추지는 않는다. 슬라이드가 도는 동안에도 중력·`PhysWalking`·스텝업·클라 예측 보정이 매 프레임 그대로 돌고, 태스크는 그 뒤에 `SetActorLocation` 으로 다시 밀어 넣는다. 세 가지가 따라온다. (a) 앵커가 지면에서 떨어져 있으면(플랫폼·계단 위 상호작용 지점) 태스크 틱 사이에 캐릭터가 낙하해 목표에 수렴하지 못한다. (b) `SetActorLocation` 이 sweep 없이 호출되어 얇은 지오메트리를 관통한다(`:554` 즉시 스냅 경로도 동일). (c) 서버와 소유 클라가 각자 같은 캐릭터를 옮기고, 그 사이 소유 클라의 CMC 는 `ServerMove` 를 계속 보내며, 시뮬레이티드 프록시에서는 `ReplicatedMovement` 보간과 이 태스크의 이동이 겹친다 — 끝점은 같아도 이동 구간의 경로는 경합한다. 완료 판정이 위치 `Equals` 하나뿐이라(`:606`) 되밀림이 생기면 상태가 길어진다.
- **제안**: 진입 시 `UCharacterMovementComponent::DisableMovement()`(또는 `SetMovementMode(MOVE_None)`)로 CMC 를 끄고, 이미 차단 해제 기록을 들고 있는 `ExitState`(`:621-639`)에서 원복해 슬라이드 구간의 이동 권위를 태스크 하나로 모은다. 최소 조치로도 `SetActorLocation(..., /*bSweep*/ true)` 로 관통을 막고 Duration 기준 타임아웃을 둔다.
- **확신도**: 중간

### 8. 🟢 저장 상태 진입 경로만 `RefreshStateTag()` 를 부르지 않아 폴백 경로와 비대칭이다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:254-259`(폴백, `:257` 에서 호출) vs `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:289-297`(저장 상태 진입, 호출 없음)
- **범주**: 버그/정확성
- **문제**: `BeginPlay` 로 들어오는 자동 시작은 `:216` 이 뒤에서 덮어 주므로 문제가 드러나지 않는다. 하지만 서버에서 `OnSaveRestored()` → `RestartLogic()`(`:140`)으로 들어와 그 `Start()` 가 곧바로 종착 상태에 도달해 트리가 멈추면, 틱도 안 돌고 `StopLogic()` 도 불리지 않아 `StateTag` 가 복원 이전 값으로 남는다. 다음 세이브에 낡은 상태가 기록된다.
- **제안**: `:292` `ScheduleTickFrame` 전후에 `RefreshStateTag()` 를 한 번 부른다(폴백 경로와 동일하게).
- **확신도**: 중간

### 9. 🟢 스포너 로케이터 태스크 2종의 `Compile` 이 완전히 동일하고, `IsInitialEntry` 판정도 재구현돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:118-137`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:207-226`
- **범주**: 중복/복잡도
- **문제**: 두 `Compile` 본문(주석 포함 19줄)이 글자 하나 다르지 않다. 검증 규칙이 바뀌면 두 곳을 함께 고쳐야 하고, 한쪽만 고치면 컴파일은 통과하되 한 태스크만 조용히 느슨해진다. 같은 파일의 `:82` 는 `!Transition.SourceStateID.IsValid()` 를 다시 적어 두었는데, 이 판정에는 이미 이름과 근거 주석을 갖춘 헬퍼가 `WxGimmickStateTreeNodes.cpp:47-50` 에 있다(파일 로컬이라 재사용이 막혀 있다).
- **제안**: `ValidateSpawnerLocators(...)` 헬퍼를 두고 두 `Compile` 이 그것만 호출하게 한다. `IsInitialEntry` 는 두 노드 파일이 함께 볼 수 있는 자리(예: 모듈 내부 헤더)로 올려 판정 문구를 하나로 모은다.
- **확신도**: 높음

### 10. 🟢 기믹 컴포넌트가 오너 액터의 `bReplicates` 를 조용히 전제하는데, 그것을 보장하던 C++ 베이스가 사라졌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:28-31`
- **범주**: 설계/구조
- **문제**: 컴포넌트는 `SetIsReplicatedByDefault(true)`(`:31`)로 자기 몫만 켠다. 오너 액터가 `bReplicates=false` 면 `StateTag` 는 한 번도 나가지 않고 `Multicast_Interact` 도 콜스페이스가 로컬로 접혀 도달하지 않아, 헤더가 상태 구동 패턴으로 서술한 클라 수렴(레이트조인·스트리밍 인·이벤트 유실, `WxGimmickStateTreeComponent.h:58-63`)이 통째로 죽는다. 커밋 `18756ef1`("기믹 하위 클래스 제거") 이전에는 기믹 C++ 액터 생성자가 이를 켜 주는 안전망이 있었으나 지금은 배치 BP 마다 디자이너가 체크박스를 기억하는 것 외에 보장이 없고, 헤더가 내세우는 확장 규약은 "아무 액터(순수 BP 포함)에 붙이면 기믹이 된다"(`h:51`)라 전제를 알릴 자리도 없다. 싱글/리슨호스트 로컬에서는 정상으로 보여 발견이 늦다.
- **제안**: `BeginPlay` 에서 `GetOwner()->GetIsReplicated()` 가 false 면 `LogWxWorld` 경고를 한 줄 남긴다(또는 `OnRegister` 의 에디터 경로에서 Map Check 경고). 최소한 헤더 확장 규약 문단과 README 「신규 기믹」 항목에 전제를 명시한다.
- **확신도**: 높음(메커니즘 확정. 현재 배치된 4종은 `.claude/asset_dump/Blueprints/BP_{Door,Elevator,CheckPoint,TreasureChest}.json` 에서 모두 `bReplicates: true` 라 발현은 없다)

### 11. 🟢 `StartTreeAtSavedState` 가 엔진 `StartTree` 를 재조립하면서 재진입 가드를 가져오지 못했다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:248-298`
- **범주**: 성능/안전
- **문제**: 시작 상태를 넣기 위해 엔진 `UStateTreeComponent::StartTree` 를 복제해 왔는데, 원본이 컨텍스트 생성 전후에 두르는 재진입 체크와 `TGuardValue` 가 빠졌다(엔진 `StateTreeComponent.cpp:181-188` — `if (CurrentlyRunningExecContext) { Error 로그; return; }` 후 `TGuardValue` 설정). 이 멤버는 `TickComponent`(`:131`,`:138`)·`StopLogic`(`:246-249`)이 「이미 실행 중인 컨텍스트가 있으면 그것을 재사용한다」를 판단하는 근거라, 태스크의 `EnterState`/`ExitState` 안에서 컴포넌트로 재진입하는 경로가 생기면 두 갈래로 깨진다 — (a) 틱 도중 `RestartLogic()` 은 엔진 가드 대신 `FStateTreeExecutionContext::Start` 안쪽 `ensureMsgf(Exec.CurrentPhase == Unset)`(엔진 `StateTreeExecutionContext.cpp:1480-1484`)에 걸려 `Failed` 를 돌려주고, `:291` 에서 `bIsRunning=false`, `:296` 이 Failed 를 브로드캐스트해 기믹 트리가 조용히 죽는다(엔진 경로는 거부만 하고 트리를 살려 둔다). (b) `Start()` 실행 중 `StopLogic()` 이 불리면 같은 `InstanceData` 위에 컨텍스트가 하나 더 만들어진다. **다만 원본대로 옮겨오는 것은 불가능하다** — `CurrentlyRunningExecContext` 는 엔진 헤더에서 `private` 이고(`StateTreeComponent.h:195-196`) 접근권이 `friend FStateTreeComponentExecutionExtension`(`:198`)에만 열려 있어 파생 클래스가 읽지도 쓰지도 못한다. 지금 저장소의 `RestartLogic` 호출부는 `OnRep_StateTag`(`:233`)·`OnSaveRestored`(`:140`) 둘뿐이고 모두 틱 밖이라 잠재 위험에 머문다.
- **제안**: 엔진 멤버를 못 쓰므로 자체 재진입 플래그를 `StartTreeAtSavedState` 진입부에 두어 중첩 호출을 로그 후 반환시키고, 헤더 주석(`WxGimmickStateTreeComponent.h:161-165`, 이미 "엔진 업그레이드 시 확인 지점"으로 표시된 자리)에 "ST 태스크의 Enter/ExitState 에서 이 컴포넌트의 Start/Stop/RestartLogic 을 부르지 말 것"을 계약으로 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 현재 재진입 경로는 없고, 엔진이 멤버를 열어 주기 전에는 완전한 대안이 없다)

### 12. 🟢 타이머에 바인딩되는 `ScanAndPush` 가 `Handle` 접두사 규칙을 따르지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:44`(바인딩), 선언은 `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:102`
- **범주**: 규칙 위반 (코딩 규칙 4 — Delegate 에 바인딩되는 Callback 은 `Handle` prefix)
- **문제**: `SetTimer(..., &UWxInteractionScannerComponent::ScanAndPush, ...)` 로 타이머 델리게이트에 직접 바인딩된다. 모듈 안에서 델리게이트/타이머에 바인딩되는 콜백은 이 한 곳뿐이라 다른 참고 사례가 없지만, 커밋 `15a04257`("모듈 리뷰 지적 정리: … 타이머 콜백 이름 …")이 `WxAbilityBase::ResumeFromHitStop` → `HandleHitStopElapsed` 로 정리하며 프로젝트 방침을 확정했으므로 같은 기준이 적용된다.
- **제안**: 타이머 바인딩용 `HandleScanTimer()` 를 두고 그 안에서 `ScanAndPush()` 를 부른다 — 이 함수는 델리게이트 외에 직접 호출부(`:48`)도 있으므로 단순 개명보다 분리가 낫다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` — 계약 교차 확인용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, 소비처 확인용으로 `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp` 도 함께 읽음
- **코딩 규칙 스캔 결과(통과)**: Copyright 첫 줄 18/18 · `Wx` prefix 전수 준수 · Wx 의존은 `WxCore` 뿐이고 역방향 참조는 `Source/WxGame/WxGame.Build.cs:36` 하나뿐(규칙 g 만족) · `BlueprintCallable` 은 `WxSpawnerLibrary.h:27` 한 곳(BP Function Library) · `FORCEINLINE` 없음 · 헤더 인라인 정의는 `GetInstanceDataType()` 16곳뿐이고 두 노드 헤더 상단(`WxGimmickStateTreeNodes.h:32`, `WxSpawnerStateTreeNodes.h:14`)이 규칙 6 예외임을 명시 · 람다는 스캐너의 거리순 정렬 술어(`WxInteractionScannerComponent.cpp:202`) 하나로 대체가 마땅치 않음 · 오버라이드 `Super::` 호출은 정상(단 `StartLogic`/`RestartLogic` 은 엔진 `StartTree` 재조립이 목적인 의도적 비호출이며, 엔진 두 함수 모두 `StartTree()` 만 부르므로 폴백 경로의 `Super::StartLogic()` 대체는 동작상 동일함을 `StateTreeComponent.cpp:160-170` 로 확인)
- **미검토 / 한계**: (1) `Content/WorldObject/Gimmick/ST_*.uasset` 은 `.claude/asset_dump/StateTrees/` JSON 으로 「어떤 태스크가 실려 있는가」만 확인했고 상태 구조·전이 배선은 열지 않았다 — 발견 1·4·6 의 발현 폭(트리거형 태스크를 클라 가시 상태에 다는지, 다중 영역 기믹이 이동 중 나머지 영역을 켜 두는지, 재선택을 받는 상태가 있는지)은 그 배선에 달려 있다. (2) 멀티플레이 실기 검증은 없다 — 발견 1 의 서버 프레임 내 실행 순서, 발견 2 의 에디터 복제 훅, 발견 7 의 CMC 경합은 엔진 코드·프로퍼티 플래그 기반 정적 분석이다. (3) `FUniversalObjectLocator::SyncFind` 를 매 틱 도는 `WaitSpawnersKilled`(`WxSpawnerStateTreeNodes.cpp:194-201`)와 0.1초 주기 전 오브젝트 채널 오버랩 + `Examined.Contains` 선형 탐색을 도는 `ScanAndPush`(`WxInteractionScannerComponent.cpp:163-199`)의 실측 비용은 재지 않았다 — 둘 다 헤더가 의도된 트레이드오프로 명시해 두어 발견에 넣지 않았다. (4) `EnablePlayerInput` 이 상호작용 당사자가 아니라 "이 머신의 첫 로컬 플레이어"를 토글하는 멀티플레이 한계(`WxGimmickStateTreeNodes.cpp:224`)는 헤더(`h:204-205`)가 미적용 사유까지 적어 둔 의식적 보류라 발견으로 올리지 않았다. (5) 에디터 전용 프리뷰 경로(`AWxSpawner::UpdateEditorPreviewFromSpawnableClass` 의 바운드 계산, `UWxWorldDeveloperSettings` 아이콘 매핑)는 읽었으나 실제 에디터에서 검증하지 않았다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 18파일 — `/module-review`로 갱신*
