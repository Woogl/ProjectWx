# WxWorld — 코드 리뷰

> 기믹의 세 책임(상호작용·영속·ST 구동)을 컴포넌트 하나로 접고 동작은 전부 ST 에셋이 author 하는 구조가 일관되게 서 있으며, 널 가드·권위 가드·초기진입/라이브 분기·차단 해제 짝맞춤이 전반적으로 성실하다. 직전 리뷰(`c37b6fa6`) 이후 C++ 기믹 액터 클래스 5종이 사라져(26→17파일) 헤더 주석 스테일 건은 해소됐으나, 그 삭제로 `bReplicates` 를 보장하던 자리가 함께 없어졌고 「멀티캐스트 이벤트 vs 복제 StateTag」 도착 순서 문제는 그대로 남았다. 이번 리뷰는 C++ 17파일 전부를 열고 기믹 컴포넌트·ST 노드 라이브러리·스캐너·스포너의 cpp 를 정독했으며, 판정 근거로 엔진 `UStateTreeComponent`·`FStateTreeExecutionContext`·`FStateTreeInstanceStorage`·`UNiagaraFunctionLibrary`(UE 5.8) 구현을 함께 확인했다. 코딩 규칙 스캔에서는 Copyright 첫 줄 17/17 · `Wx` prefix · Wx 의존은 `WxCore` 뿐 · `BlueprintCallable` 은 BP Function Library 1곳으로 모두 통과했고, 인라인 정의 1건만 걸렸다(발견 7).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 `OnRep_StateTag` 가 아직 처리되지 않은 대기 이벤트를 "어긋난 피어"로 오판해, 클라에서 트리거형 연출이 통째로 스킵된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:219-234`(OnRep), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:236-246`(Multicast)
- **범주**: 버그/정확성
- **문제**: `Multicast_Interact_Implementation` 의 `SendStateTreeEvent`(`:245`)는 이벤트를 **큐에 넣기만** 한다(엔진 `StateTreeComponent.cpp:577-593` → `FStateTreeMinimalExecutionContext::SendEvent`, 실제 전이는 다음 트리 틱). 반면 `OnRep_StateTag` 는 그 자리에서 `GetActiveStateTag() == StateTag` 를 비교하므로(`:228`), 멀티캐스트 RPC 와 `StateTag` 프로퍼티가 같은 네트 업데이트로 도착하면 판정 시점의 로컬 트리는 아직 **이전 상태**다 → 불일치로 보고 `RestartLogic()`(`:233`)을 탄다. 재시작은 `FStateTreeExecutionContext::Start` 안에서 `Stop()` 후 `InstanceData.Reset()` 을 수행하고(엔진 `StateTreeExecutionContext.cpp:1487-1493`), 그 `Reset` 이 `EventQueue->Reset()` 으로 대기 큐를 비운다(엔진 `StateTreeInstanceData.cpp:769-786`). 즉 **큐에 있던 Interact 이벤트가 폐기되고**, 새 진입은 `SourceStateID` 가 무효인 초기 진입이 되어 `IsInitialEntry` 로 갈리는 트리거형 태스크가 전부 스킵된다 — PlaySound(`WxGimmickStateTreeNodes.cpp:834-835`), PlayLevelSequence(`:757`), PlayInteractorMontage(`:666`), MoveInteractorToTarget(`:501`), ApplyGameplayEffectToInteractor(`:128`). 서버 타임라인상 두 데이터는 같은 프레임에 생성된다(TickDispatch 에서 어빌리티→`OnInteracted`→멀티캐스트 송출, 같은 프레임 액터 틱에서 ST 전이→`RefreshStateTag`→`StateTag` 변경, 프레임 말 복제). 즉 드문 경합이 아니라 상시 발현일 수 있으며, 리슨호스트 로컬에서는 정상으로 보여 PIE 단일 플레이 테스트로는 드러나지 않는다.
- **제안**: OnRep 시점에 즉시 판정하지 않는다 — (a) 대기 중인 `StateTree.Interact` 이벤트가 있으면 이번 판정을 건너뛰거나, (b) 판정을 다음 틱 말미(`RefreshStateTag` 와 같은 자리)로 미뤄 로컬 트리가 큐를 소화한 뒤에 비교하거나, (c) 복제 대상에 "직전 전이 시퀀스 번호"를 추가해 「아직 못 따라온 것」과 「진짜 어긋난 것」을 구분한다. 검증은 PIE 2-클라이언트에서 사운드/컷신 태스크가 걸린 기믹을 리모트 클라 화면으로 확인하면 즉시 갈린다.
- **확신도**: 중간(코드 경로는 엔진 구현까지 확인. 패킷 동시 도착 가정만 실측 미확인이며, 한 프레임 이상 벌어져 도착하면 정상 경로로 수렴한다)

### 2. 🟡 Interactor 계열 태스크가 대상 캐릭터를 매 틱 라이브 재조회해, 도중에 당사자가 바뀌면 엉뚱한 캐릭터를 옮긴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:574-584`(Move Tick), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:690-709`(Montage Tick)
- **범주**: 버그/정확성
- **문제**: 두 태스크의 `Tick` 은 매 프레임 `Gimmick->GetInteractingCharacter()` 를 다시 읽는다(`:580`, `:695`). 그런데 `InteractingCharacter` 는 권위 측이 상호작용마다 멀티캐스트로 덮어쓰는 라이브 멤버다(`WxGimmickStateTreeComponent.cpp:239`) — 같은 파일의 `ExitState` 주석(`:624-626`)이 정확히 이 이유로 **차단 대상만은** 인스턴스 데이터에 스냅샷해 두는데(`:534`, `:539`), 정작 이동/판정 대상은 스냅샷하지 않는다. 이동 중 다른 플레이어가 같은 기믹의 (아직 켜져 있는) 다른 영역을 누르면, `MoveSpeed`/`TurnSpeed` 는 A 기준으로 산출된 채 B 를 `SetActorLocation` 으로 끌고 가고(A 는 그 자리에 남되 입력 차단만 짝이 맞아 풀린다), `PlayInteractorMontage` 는 재생한 적 없는 B 의 몽타주를 폴링해 `Montage_IsPlaying == false` 로 즉시 Succeeded 를 반환해 연출이 조기 종료된다. 엘리베이터처럼 영역이 여럿이라 한 상태에서 일부 영역만 끄는 기믹이 이 조건에 해당한다.
- **제안**: `BlockedController` 와 동형으로 `EnterState` 에서 대상 캐릭터를 `TWeakObjectPtr` 인스턴스 데이터에 기록하고, `Tick`/완료 판정은 그 기록만 근거로 쓴다(기록이 죽었으면 Failed 대신 완료로 빠져 상태가 갇히지 않게).
- **확신도**: 중간(재조회 자체는 코드로 확정. 실제 발현은 각 ST 에셋이 그 상태에서 나머지 영역을 끄는지에 달려 있어 미확인)

### 3. 🟡 `AWxSpawner` 의 SaveId 부여 훅이 에디터 복제 경로를 놓쳐, 복제된 스포너끼리 세이브 키가 충돌한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:192-205`
- **범주**: 버그/정확성
- **문제**: `SaveId` 를 `PostActorCreated`/`PostDuplicate` 두 훅에서만 심는다. 그런데 에디터의 액터 복제(Ctrl+D·Alt-드래그·복사/붙여넣기)는 `StaticDuplicateObject` 가 아니라 텍스트 export/import 경로다 — 새 액터를 `SpawnActor` 로 만든 뒤(`PostActorCreated` 통과) 원본 T3D 를 `ImportObjectProperties` 로 덮어쓰므로, 플래그 없는 평범한 `UPROPERTY` 인 `SaveId`(`WxSpawner.h:82-84`)는 **원본 값으로 되돌아가고** `PostDuplicate` 는 불리지 않아 교정 기회도 없다. 반면 `ActorGuid` 는 직렬화 단계에서 새로 발급된다. 결과적으로 복제한 스포너 둘이 같은 `SaveId` 로 한 슬롯 레코드를 공유하므로, 한쪽 보스를 잡으면 다른 쪽도 처치 상태로 복원된다(`bIsKilled` 는 `SaveGame`, `WxSpawner.h:78-80`). 같은 이유로 이 코드 이전에 배치된 스포너는 `SaveId` 가 무효인 채 남아 저장/복원에서 조용히 제외된다. 같은 모듈의 기믹 컴포넌트는 이 문제를 이미 다르게 풀었다 — `OnRegister` 에서 매 등록마다 오너 `ActorGuid` 와 대조해 신규 배치·복제·기존 마이그레이션을 한 경로로 처리하고, 그 주석(`WxGimmickStateTreeComponent.cpp:61-62`)이 액터 단위 사전 훅을 쓸 수 없는 이유까지 적어 두었다.
- **제안**: `WxGimmickStateTreeComponent::OnRegister`(`:49-73`)와 동일한 패턴으로 통일한다 — 에디터 월드에서 등록할 때마다 `GetActorGuid()` 와 비교해 다르면 `Modify()` 후 갱신(같으면 노옵). 두 훅은 그때 제거할 수 있다.
- **확신도**: 중간(값 복사와 ActorGuid 재발급은 엔진 코드로 확인. 에디터 복제가 `PostDuplicate` 를 타지 않는다는 점은 경로 추론이라 실측 권장)

### 4. 🟡 `SpawnNiagara` 에 정리 경로가 없어, 상태를 떠나도 FX 가 그대로 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:873-905`, 선언은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:605-620`
- **범주**: 버그/정확성
- **문제**: 이 태스크는 `EnterState` 만 가지고 `ExitState` 가 없다. 헤더는 "루프 Niagara 를 지정하면 **상태에 묶인** 지속 FX 가 된다"(`h:601`)고 계약을 서술하지만, 상태를 떠나도 `SpawnedComponent` 를 정지·파괴하는 자리가 어디에도 없어 FX 는 계속 재생된다 — 즉 켤 수는 있어도 끌 수가 없다. 두 스폰 경로 모두 `bAutoDestroy=true` 지만(`cpp:896`, `:900` — 엔진 `NiagaraFunctionLibrary.h:93,96`) 이는 시스템이 **완료될 때** 파괴한다는 뜻이라 루프 시스템에는 영원히 발동하지 않는다. 게다가 `AttachComponent` 를 비운 경로는 `SpawnSystemAtLocation` 으로 월드 소유 컴포넌트를 만들어 오너에 붙지 않으므로, 기믹 액터가 파괴되거나 WP 셀이 언로드돼도 FX 가 월드에 남는다. 지금 체크포인트 모닥불(종착 상태라 이탈이 없음)에서만 쓰여 발현하지 않을 뿐, 상태 A 에서 켜고 B 에서 꺼야 하는 첫 기믹에서 바로 드러난다. 같은 파일의 `PlayLevelSequence`(`:803-808`)·`SpawnActor`(`:1042-1065`)는 둘 다 `ExitState` 로 자기 산출물을 회수한다.
- **제안**: `ExitState` 를 추가해 `SpawnedComponent` 가 유효하면 `Deactivate()`(또는 `DestroyComponent()`) 후 핸들을 비운다. 지속 FX 를 이탈 후에도 남기고 싶은 경우가 있다면 `SpawnActor` 의 `bDestroyOnExit` 처럼 인스턴스 데이터 플래그로 노출한다.
- **확신도**: 중간(정리 경로 부재는 코드로 확정. "상태 이탈 시 남는 것이 의도"일 가능성은 있으나 헤더 서술과 형제 태스크 관례 양쪽과 어긋난다)

### 5. 🟡 진행 중인 작업을 다루는 Interactor 태스크 2종이 `bShouldStateChangeOnReselect` 를 선언하지 않아, 재선택에 연출이 처음부터 다시 돈다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:429-444`(MoveInteractorToTarget), `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:465-479`(PlayInteractorMontage)
- **범주**: 설계/구조
- **문제**: 두 태스크만 생성자가 아예 없어 엔진 기본값 `true` 를 그대로 쓴다(엔진 `StateTreeTaskBase.h:24`). 그런데 같은 헤더가 그 정책을 이렇게 못박아 두었다 — "발동 순간에 한 번 일어나는 액션형은 true, 진행 중인 작업이 재선택에 끊기지 않아야 하는 것은 false"(`h:56-58`). 실제로 형제 태스크는 전부 false 를 선언한다: ComponentMove(`cpp:280`), ComponentSplineMove(`cpp:358`), PlayLevelSequence(`cpp:749`), SpawnActor(`cpp:974`), WaitSpawnersKilled(`WxSpawnerStateTreeNodes.cpp:153`). 재선택(Sustained)이 걸리면 `MoveInteractorToTarget` 은 `ExitState`(입력 차단 해제)→`EnterState`(재차단·`MoveSpeed` 재산출)로 이동이 현재 위치부터 다시 시작돼 도착이 늦어지고, `PlayInteractorMontage` 는 몽타주를 처음부터 다시 튼다 — 재선택이 반복되면 끝나지 않는다. 헤더 자신이 "루트 재선택 thrash"(`h:61`)를 실재하는 위험으로 경고하는 만큼 가정만의 시나리오가 아니다.
- **제안**: 두 태스크에 생성자를 추가해 `bShouldStateChangeOnReselect = false` 를 선언한다. `PlayAnimation`(`h:357-371`)도 같은 모양이지만 헤더가 "진입 경로 무관 처음부터 재생"을 명시적 방침으로 적어 두었으므로 함께 판단해 결정한다.
- **확신도**: 중간(기본값과 형제 관례는 코드로 확정. 현재 ST 에셋에서 이 두 태스크가 실린 상태가 재선택을 받는지는 미확인)

### 6. 🟡 기믹 컴포넌트가 오너 액터의 `bReplicates` 를 조용히 전제하는데, 그것을 보장하던 C++ 베이스가 사라졌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:28-31`
- **범주**: 설계/구조
- **문제**: 컴포넌트는 `SetIsReplicatedByDefault(true)` 로 자기 몫만 켠다. 오너 액터가 `bReplicates=false` 면 `StateTag` 는 한 번도 나가지 않고 `Multicast_Interact` 도 도달하지 않아, 헤더가 상태 구동 패턴으로 서술한 클라 수렴(레이트조인·스트리밍 인·이벤트 유실, `WxGimmickStateTreeComponent.h:58-63`)이 통째로 죽는다. 직전 리뷰 시점에는 `AWxGimmick` 생성자가 이를 켜 주는 안전망이 있었으나, 2026-08-03 기믹 C++ 액터 클래스가 전부 삭제되어(`README.md:31-33` "기믹용 C++ 액터 클래스는 없다") 지금은 배치 BP 마다 디자이너가 체크박스를 기억하는 것 외에 보장이 없다. 헤더가 내세우는 확장 규약은 "아무 액터(순수 BP 포함)에 붙이면 기믹이 된다"(`h:51`)이므로 전제를 알릴 자리도 없다. 현재 배치된 4종(BP_Door·BP_Elevator·BP_TreasureChest·BP_CheckPoint)은 모두 `bReplicates=true` 라 지금 당장의 발현은 없고, 싱글/리슨호스트 로컬에서는 정상으로 보여 발견이 늦다.
- **제안**: `BeginPlay` 에서 `GetOwner()->GetIsReplicated()` 가 false 면 `LogWxWorld` 경고를 한 줄 남긴다(또는 `OnRegister` 의 에디터 경로에서 Map Check 경고). 최소한 헤더 확장 규약 문단과 README 「신규 기믹」 항목에 전제를 명시한다.
- **확신도**: 높음(메커니즘 확정. 현재 배치 에셋 기준 발현은 없음)

### 7. 🟢 스캐너 헤더의 인라인 함수 정의 — 코딩 규칙 6 위반
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:57`
- **범주**: 규칙 위반
- **문제**: `int32 GetSelectedIndex() const { return SelectedIndex; }` 가 헤더에 본문을 들고 있다. 같은 모듈의 다른 헤더 인라인 정의 16곳은 전부 ST 노드의 `GetInstanceDataType` 으로, 그 헤더들이 "코딩 규칙 6 의 예외"임을 명시해 두었지만(`WxGimmickStateTreeNodes.h:32`, `WxSpawnerStateTreeNodes.h:14`) 이 접근자는 그 예외에 해당하지 않는다. 직전 커밋 `14a77aef` 가 WxUI 헤더의 같은 종류를 cpp 로 옮긴 만큼 프로젝트의 현재 방침도 분명하다.
- **제안**: 선언만 남기고 정의를 `WxInteractionScannerComponent.cpp` 로 옮긴다(헤더 선언 순서대로 배치).
- **확신도**: 높음

### 8. 🟢 스포너 로케이터 태스크 2종의 `Compile` 이 완전히 동일하고, `IsInitialEntry` 판정도 재구현돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:118-137`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp:207-226`
- **범주**: 중복/복잡도
- **문제**: 두 `Compile` 본문(주석 포함 19줄)이 글자 하나 다르지 않다. 검증 규칙이 바뀌면 두 곳을 함께 고쳐야 하고, 한쪽만 고치면 컴파일은 통과하되 한 태스크만 조용히 느슨해진다. 같은 파일의 `:82` 는 `!Transition.SourceStateID.IsValid()` 를 다시 적어 두었는데, 이 판정에는 이미 이름과 근거 주석을 갖춘 헬퍼가 `WxGimmickStateTreeNodes.cpp:47-50` 에 있다(파일 로컬 익명 네임스페이스라 재사용이 막혀 있다).
- **제안**: 파일 익명 네임스페이스에 `ValidateSpawnerLocators(...)` 헬퍼를 두고 두 `Compile` 이 그것만 호출하게 한다. `IsInitialEntry` 는 두 노드 파일이 함께 볼 수 있는 자리(예: 모듈 내부 헤더)로 올려 판정 문구를 하나로 모은다.
- **확신도**: 높음

### 9. 🟢 `StartTreeAtSavedState` 가 엔진 `StartTree` 를 재조립하면서 재진입 가드를 가져오지 못했다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:248-298`
- **범주**: 성능/안전
- **문제**: 시작 상태를 넣기 위해 엔진 `UStateTreeComponent::StartTree` 를 복제해 왔는데, 원본이 컨텍스트 생성 전후에 두르는 `CurrentlyRunningExecContext` 체크와 `TGuardValue` 가 빠졌다(엔진 `StateTreeComponent.cpp:174-188`). 이 멤버는 `TickComponent`·`StopLogic` 이 「이미 실행 중인 컨텍스트가 있으면 그것을 재사용한다」를 판단하는 근거라, 태스크의 `EnterState`/`ExitState` 안에서 컴포넌트로 재진입하는 경로가 생기면 같은 `InstanceData` 위에 두 번째 실행 컨텍스트가 만들어진다. **다만 원본대로 옮겨오는 것은 불가능하다** — `CurrentlyRunningExecContext` 는 엔진 헤더에서 `private` 이라(`StateTreeComponent.h:195-196`) 파생 클래스가 읽지도 쓰지도 못한다(직전 리뷰가 제시한 "두 줄 이식" 제안은 컴파일되지 않는다). 지금 노드 집합에는 `StopLogic`/`RestartLogic` 을 부르는 태스크가 없어 잠재 위험에 머문다.
- **제안**: 코드로 막을 수 없으므로 계약으로 못박는다 — 헤더 주석(`WxGimmickStateTreeComponent.h:161-165`, 이미 "엔진 업그레이드 시 확인 지점"으로 표시된 자리)에 "ST 태스크의 Enter/ExitState 에서 이 컴포넌트의 Start/Stop/RestartLogic 을 부르지 말 것"을 명시하고, 꼭 필요하면 다음 틱으로 지연시킨다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 현재 재진입 경로는 없고, 엔진이 멤버를 열어 주기 전에는 대안이 없다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` — 계약 교차 확인용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, 소비처 확인용으로 `Source/WxGame/Character/WxEnemyCharacter.cpp`(`MarkKilled`·`OnSpawnedBy`), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(스캐너 구독) 도 함께 읽음
- **미검토 / 한계**: (1) `Content/WorldObject/Gimmick/ST_*.uasset` 은 열지 않았다 — 발견 1·2·5 의 실제 발현 폭(어느 상태가 트리거형 태스크를 쓰는지, 이동 상태에서 다른 영역을 끄는지, 재선택을 받는 상태가 있는지)은 에셋에 달려 있다. 발견 6 의 배치 BP `bReplicates` 값만 `.claude/asset_dump/Blueprints/*.json` 으로 확인했다. (2) 멀티플레이 실기 검증은 없다. 발견 1 의 패킷 동시 도착, 발견 3 의 에디터 복제 훅은 엔진 코드 기반 추론이다. (3) `FUniversalObjectLocator::SyncFind` 를 매 틱 도는 `WaitSpawnersKilled`(`WxSpawnerStateTreeNodes.cpp:194-201`)와 0.1초 주기 전 오브젝트 채널 오버랩 + `Examined.Contains` 선형 탐색을 도는 `ScanAndPush`(`WxInteractionScannerComponent.cpp:158-194`)의 실측 비용은 재지 않았다 — 둘 다 헤더가 의도된 트레이드오프로 명시해 두어 발견에 넣지 않았다. (4) `EnablePlayerInput` 이 상호작용 당사자가 아니라 "이 머신의 첫 로컬 플레이어"를 토글하는 멀티플레이 한계(`WxGimmickStateTreeNodes.cpp:224`)는 헤더(`h:204-205`)가 이미 미적용 사유까지 정확히 적어 둔 의식적 보류라 발견으로 올리지 않았다. (5) 타이머에 바인딩되는 `ScanAndPush`(`WxInteractionScannerComponent.cpp:44`)의 `Handle` prefix 부재는 규칙 판정이 경계이고(프로젝트 내 타이머 콜백 명명이 `HandleGroggySafetyTimeout`/`TickPlayMontage` 로 이미 갈려 있다) 발견으로 올리지 않았다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 17파일 — `/module-review`로 갱신*
