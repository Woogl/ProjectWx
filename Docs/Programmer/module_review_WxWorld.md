# WxWorld — 코드 리뷰

> 기믹의 세 책임(상호작용·영속·ST 구동)을 컴포넌트 하나로 접고 동작은 전부 ST 에셋이 author 하는 구조가 일관되게 서 있고, 널 가드·권위 가드·초기진입/라이브 분기·차단 해제 짝맞춤이 전반적으로 성실하다. 직전 리뷰(`00b2e3f4`)의 8건은 아키텍처 재편과 함께 대부분 해소됐으며, 남은 위험은 「멀티캐스트 이벤트 vs 복제 StateTag」의 도착 순서와 라이브 멤버를 매 틱 재조회하는 태스크 2종에 몰려 있다. 이번 리뷰는 C++ 26파일 전부를 열고 기믹 컴포넌트·ST 노드 라이브러리·스캐너·스포너의 cpp 를 정독했으며, 판정 근거로 엔진 `UStateTreeComponent`·`FStateTreeExecutionContext`(UE 5.8) 구현을 함께 확인했다. 코딩 규칙 스캔(Copyright 첫 줄 26/26 · `Wx` prefix · Wx 의존은 `WxCore` 뿐 · `BlueprintCallable` 은 BP Function Library 1곳 · 람다 1곳은 캡처 필요 정렬 술어)에서 위반은 없다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 `OnRep_StateTag` 가 아직 처리되지 않은 대기 이벤트를 "어긋난 피어"로 오판해, 클라에서 트리거형 연출이 통째로 스킵될 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:219-234`(OnRep), `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:236-246`(Multicast)
- **범주**: 버그/정확성
- **문제**: `Multicast_Interact_Implementation` 의 `SendStateTreeEvent`(`:245`)는 이벤트를 **큐에 넣기만** 한다(엔진 `StateTreeComponent.cpp:577-593` → `FStateTreeMinimalExecutionContext::SendEvent`, 실제 전이는 다음 트리 틱). 반면 `OnRep_StateTag` 는 그 자리에서 `GetActiveStateTag() == StateTag` 를 비교하므로(`:228`), 멀티캐스트 RPC 와 `StateTag` 프로퍼티가 같은 네트 업데이트로 도착하면 판정 시점의 로컬 트리는 아직 **이전 상태**다 → 불일치로 보고 `RestartLogic()`(`:233`)을 탄다. 재시작은 `FStateTreeExecutionContext::Start` 안에서 `Stop()` 후 `InstanceData.Reset()`(엔진 `StateTreeExecutionContext.cpp:1487-1493`)을 수행하므로 **큐에 있던 Interact 이벤트가 폐기되고**, 새 진입은 `SourceStateID` 가 무효인 초기 진입이 되어 `IsInitialEntry` 로 갈리는 트리거형 태스크가 전부 스킵된다 — PlaySound(`WxGimmickStateTreeNodes.cpp:835`), PlayLevelSequence(`:757`), PlayInteractorMontage(`:666`), MoveInteractorToTarget(`:501`). 서버 타임라인상 두 데이터는 같은 프레임에 생성된다(TickDispatch 에서 어빌리티→`OnInteracted`→멀티캐스트 송출, 같은 프레임 액터 틱에서 ST 전이→`RefreshStateTag`→`StateTag` 변경, 프레임 말 복제). 즉 이 경로는 드문 경합이 아니라 상시 발현일 수 있으며, 리슨호스트 로컬에서는 정상으로 보여 PIE 단일 플레이 테스트로는 드러나지 않는다.
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
- **문제**: `SaveId` 를 `PostActorCreated`/`PostDuplicate` 두 훅에서만 심는다. 그런데 에디터의 액터 복제(Ctrl+D·Alt-드래그·복사/붙여넣기)는 텍스트 export/import 경로라 `PostDuplicate` 가 아니라 `PostEditImport` 를 태우고, `SaveId` 는 평범한 `UPROPERTY` 라 원본 값이 그대로 복사된다 — 반면 `ActorGuid` 는 직렬화 단계에서 새로 발급된다(엔진 `Actor.cpp:1022-1023`). 결과적으로 복제한 스포너 둘이 같은 `SaveId` 를 들고 한 슬롯 레코드를 공유하므로, 한쪽 보스를 잡으면 다른 쪽도 처치 상태로 복원된다(`bIsKilled` 는 `SaveGame` 이므로 그대로 걸린다, `WxSpawner.h:79-84`). 같은 이유로 이 코드 이전에 배치된 스포너는 `SaveId` 가 무효인 채 남아 저장/복원에서 조용히 제외된다. 같은 모듈의 기믹 컴포넌트는 이 문제를 이미 다르게 풀었다 — `OnRegister` 에서 매 등록마다 오너 `ActorGuid` 와 대조해 신규 배치·복제·기존 마이그레이션을 한 경로로 처리하고, 그 주석(`WxGimmickStateTreeComponent.cpp:61-62`)이 액터 단위 사전 훅을 쓸 수 없는 이유까지 적어 두었다.
- **제안**: `WxGimmickStateTreeComponent::OnRegister`(`:49-73`)와 동일한 패턴으로 통일한다 — 에디터 월드에서 등록할 때마다 `GetActorGuid()` 와 비교해 다르면 `Modify()` 후 갱신(같으면 노옵). 두 훅은 그때 제거할 수 있다.
- **확신도**: 중간(값 복사와 ActorGuid 재발급은 엔진 코드로 확인. 에디터 복제가 `PostDuplicate` 를 타지 않는다는 점은 경로 추론이라 실측 권장)

### 4. 🟢 기믹 컴포넌트가 오너 액터의 `bReplicates` 를 조용히 전제한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:28-31`
- **범주**: 설계/구조
- **문제**: 컴포넌트는 `SetIsReplicatedByDefault(true)` 로 자기 몫만 켠다. 오너 액터가 `bReplicates=false` 면 `StateTag` 는 한 번도 나가지 않아 클라 수렴(레이트조인·스트리밍 인·이벤트 유실)이 통째로 죽는다. `AWxGimmick` 은 생성자에서 이를 켜 두지만(`WxGimmick.cpp:13`), 헤더와 README 가 내세우는 확장 규약은 "아무 액터(순수 BP 포함)에 붙이면 기믹이 된다"(`WxGimmickStateTreeComponent.h:51`)여서 그 경로에는 전제를 알릴 자리가 없다. 싱글/리슨호스트 로컬에서는 정상 동작하므로 발견이 늦다.
- **제안**: `BeginPlay` 에서 `GetOwner()->GetIsReplicated()` 가 false 면 `LogWxWorld` 경고를 한 줄 남긴다(또는 헤더 확장 규약 문단에 명시).
- **확신도**: 높음(메커니즘 확정, 실제 배치가 전부 `AWxGimmick` 파생이면 현재 발현은 없음)

### 5. 🟢 `StartTreeAtSavedState` 가 엔진 `StartTree` 를 재조립하면서 재진입 가드를 빠뜨렸다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:248-298`
- **범주**: 성능/안전
- **문제**: 시작 상태를 넣기 위해 엔진 `UStateTreeComponent::StartTree` 를 복제해 왔는데, 원본이 컨텍스트 생성 전후에 두르는 `CurrentlyRunningExecContext` 체크와 `TGuardValue` 가 빠졌다(엔진 `StateTreeComponent.cpp:181-188`). 이 멤버는 `TickComponent`·`StopLogic` 이 「이미 실행 중인 컨텍스트가 있으면 그것을 재사용한다」를 판단하는 근거라, 태스크의 `EnterState`/`ExitState` 안에서 컴포넌트로 재진입하는 경로가 생기면(액터 파괴 → `EndPlay` → `StopLogic` 등) 같은 `InstanceData` 위에 두 번째 실행 컨텍스트가 만들어진다. 지금 노드 집합에는 그런 호출이 없어 현재는 잠재 위험이며, 헤더가 이미 "엔진 업그레이드 시 확인 지점"(`WxGimmickStateTreeComponent.h:163`)이라 적어 둔 자리다.
- **제안**: 엔진 원본과 동일하게 `if (CurrentlyRunningExecContext) { return; }` + `TGuardValue<FStateTreeExecutionContext*>` 를 함께 옮겨 온다(두 줄).
- **확신도**: 낮음(의도된 설계일 수 있음 — 현재 재진입 경로는 없음)

### 6. 🟢 헤더가 존재하지 않는 통지와 이미 해소된 "휴면" 상태를 계약으로 서술한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:46`·`:203`·`:503`
- **범주**: 중복/복잡도
- **문제**: (a) `:46`·`:503` 은 PlayLevelSequence 가 "권위 측이면 소유 기믹의 `HandleLevelSequenceFinished` 로 통지한 뒤 Succeeded" 라고 적었지만, 그 함수는 저장소 어디에도 없고 구현(`WxGimmickStateTreeNodes.cpp:788-801`)은 정리 후 Succeeded 만 반환한다(2026-06-21 워크로그에서 베이스 훅이 제거됐다). (b) `:203` 은 EnablePlayerInput 의 한계로 "당사자로 좁히려면 `InteractingCharacter` 가 필요한데 그 값을 채우는 배선(`SetInteractingCharacter` 호출부)이 아직 없어 보류"라고 적었으나, 지금은 `Multicast_Interact_Implementation`(`WxGimmickStateTreeComponent.cpp:239`)이 전 피어에 채우고 있어 보류 사유가 이미 사라졌다. 이 헤더 주석이 신규 태스크·기믹 작성의 정본이라 오해 비용이 크고, 특히 (b)는 발견 1·2 와 맞물린 실제 개선 여지를 가린다.
- **제안**: (a)는 "종료 시 정리하고 Succeeded — 상태 완료 전이가 다음을 잇는다"로 정정하고, (b)는 "당사자 지정으로 좁힐 수 있다(미적용)"로 고쳐 남은 것이 배선이 아니라 판단임을 밝힌다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerStateTreeNodes.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxDoor.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxElevator.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxTreasureChest.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxTreasureChest.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCheckPoint.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCheckPoint.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` — 계약 교차 확인용으로 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, 소비처 확인용으로 `Source/WxGame/Character/WxEnemyCharacter.cpp`(`MarkKilled`·`OnSpawnedBy`), `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`(스캐너 구독 해제) 도 함께 읽음
- **미검토 / 한계**: (1) `Content/WorldObject/Gimmick/ST_*.uasset` 은 열지 않았다 — 발견 1·2 의 실제 발현 폭(어느 상태가 트리거형 태스크를 쓰는지, 이동 상태에서 다른 영역을 끄는지)은 에셋에 달려 있다. (2) 멀티플레이 실기 검증은 없다. 발견 1 의 패킷 동시 도착, 발견 3 의 에디터 복제 훅은 엔진 코드 기반 추론이다. (3) `FUniversalObjectLocator::SyncFind` 를 매 틱 도는 `WaitSpawnersKilled`(`WxSpawnerStateTreeNodes.cpp:194-201`)의 실측 비용은 재지 않았다 — 헤더가 의도된 트레이드오프로 명시해 두어 발견에 넣지 않았다. (4) 규칙 판정 중 두 건은 경계라 발견으로 올리지 않았다 — 타이머에 바인딩되는 `ScanAndPush`(`WxInteractionScannerComponent.cpp:44`)의 `Handle` prefix 부재(델리게이트 콜백보다 스케줄 함수에 가깝다고 봄), 헤더 인라인 정의 15곳(그중 14곳은 ST 노드의 `GetInstanceDataType` 으로 엔진 강제 패턴이며 전 Wx 모듈 공통이라 모듈 단위 지적이 부적절하다고 봄).

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 26파일 — `/module-review`로 갱신*
