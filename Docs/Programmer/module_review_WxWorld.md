# WxWorld — 코드 리뷰

> 모듈 경계(`WxCore` 외 Wx 플러그인 무참조)와 CLAUDE.md 코딩 규칙은 58파일 전부에서 지켜지고 있다. 직전 리뷰(`4096004a4`) 이후 소스 변경이 없어 이전 발견 9건은 모두 그대로 남아 있음을 현재 코드와 UE 5.8.1 엔진 소스로 다시 확인했다(라인 번호 동일). 이번에 헤더 계약 불일치 1건을 더 찾았다. 결함은 대부분 장치 동기화 컴포넌트의 "초기 상태 판정·EndPlay 플래그 수명", 스포너 정리 범위, 연출 태스크의 복원·수명 규약에 모여 있고, 특정 저작이나 스트리밍 조건에서 드러나는 잠재 결함이다.
> 커버리지: 장치 동기화·상호작용 표면·스캐너·대기 등록부·스포너·연출 태스크는 cpp 까지 깊게 봤고, 나머지 시스템·설정 파일은 훑었다. 태스크별 실제 사용 여부는 `Content` 에셋 문자열 검색으로 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 8 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 같은 틱에 다른 태그 상태로 완료 전이하는 상태를 `InitialState` 로 지정하면 권위 장치가 발행과 일회성 실행을 영구히 멈춘다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:239-253`, `:151`, `:143`, `:393-397`
- **범주**: 버그/정확성
- **문제**: 초기 상태 도달 판정은 틱이 끝난 시점에 `LastEnteredTag == InitialTarget`(`:241`) 하나만 본다. 그런데 엔진은 `EnterState` 가 완료를 반환하면 같은 틱 안에서 다음 전이를 최대 5회까지 이어 적용한다(엔진 `StateTreeExecutionContext.cpp:1979-2032`). 따라서 복원 진입에서 곧바로 끝나 완료 전이로 다른 태그 상태로 넘어가는 상태를 `InitialState` 로 지정하면, 대상 진입을 실제로 거쳤는데도(`HandleBeginApplyTransition` 이 전이 직전에 관측한다) 틱 끝의 태그가 달라 매번 불일치로 판정된다. 요청 세 번 뒤 `FailSynchronization`(`:393-397`)이 걸리지만 `InitialTarget` 은 비워지지 않아 증상이 풀리지 않는다.
  - (a) `Synchronize` 가 매 틱 `:249-250` 에서 return 해 `PublishAuthorityState`(`:253`)에 닿지 못한다. `StopLogic` 발행(`:143`)도 같은 조건에 막혀 클라이언트는 이 장치의 스냅샷을 한 번도 받지 못한다.
  - (b) `TickComponent` 가 매 틱 `bRestoringState` 를 참으로 세운다(`:151`). 이후 모든 라이브 전이가 복원으로 취급되어 `SendEvent`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners` 가 권위에서 실행되지 않고, 이동 태스크는 슬라이드 대신 스냅한다. 싱글플레이에서도 똑같다.

  현재 콘텐츠에서 `InitialState` 를 바꾼 배치는 `SiegeCannonEmplacement01` 레벨 인스턴스의 피스톤 13개(`Device.Piston.Off`)이고, `ST_Piston` 에는 `Delay` 태스크와 `OnStateCompleted` 전이가 들어 있다. 그 상태가 복원 진입 틱 안에 끝나 `Device.Piston.On` 으로 넘어가는 구조라면 이 결함이 바로 발생한다. 에디터 드롭다운(`:464-480`)은 이런 상태를 거르지 않는다.
- **제안**: 성공 판정을 "요청 이후 대상 태그 진입을 관측했는가"로 바꾼다. 권위에서는 `ObserveActiveState` 가 진입을 기록하는 자리(`:213`)에서 `InitialTarget` 과 비교해 비우면 새 멤버 없이 해결된다. 초기 상태 요청이 실패로 끝났을 때도 `InitialTarget` 을 비워 발행·라이브 실행으로 돌아가게 한다.
- **확신도**: 높음 — 코드와 엔진 전이 루프로 확인했다. 피스톤 배치에서 실제로 발생하는지는 `ST_Piston` 의 `Off` 상태 구성을 에셋에서 확인해야 한다.

### 2. 🟡 스포너 정리가 무관한 부착 액터까지 파괴하고, 체크포인트 휴식이 이를 월드 전체로 넓힌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:171-180`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:23-36`
- **범주**: 버그/정확성
- **문제**: `DestroySpawnedActor` 는 추적 중인 `SpawnedActor` 를 파괴한 뒤, 스포너에 직접 부착된 액터를 종류나 생성 주체와 상관없이 전부 `Destroy()` 한다. 이 경로는 `Respawn()`(`:59`)과 `EndPlay()`(`:102`) 양쪽에서 돈다. `TryRespawnAll` 은 Manual 이 아닌 월드의 모든 스포너에 `Respawn()` 을 부르고, 이 함수는 `ST_CheckPoint` 가 쓰는 `RespawnSpawners` 태스크(`Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RespawnSpawners.cpp:31`)와 `Source/WxGame/Framework/WxRespawnLibrary.cpp:78` 에서 호출된다. 그래서 체크포인트 휴식 한 번에, 스포너에 붙여 둔 배치 액터가 레벨 곳곳에서 함께 사라지고 레벨을 다시 읽기 전까지 돌아오지 않는다. 정찰 규약(`Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h:28`)은 직접 배치한 적도 경로를 그린 액터에 부착해 정찰하도록 허용하므로, 스포너의 경로를 공유하려고 그 스포너에 부착한 배치 적이 대표적인 피해 대상이다.

  주석(`:171`)이 근거로 드는 "스스로 부착하는 적"은 `AWxEnemyCharacter::OnSpawnedBy`(`Source/WxGame/Character/WxEnemyCharacter.cpp:94`)인데, 그 인스턴스는 바로 뒤에 `SpawnTarget()` 이 `SpawnedActor` 에 담는 같은 객체다(`:151`). 약참조가 무효해지는 경우는 액터가 파괴된 경우뿐이고 파괴된 액터는 부착에서도 풀리므로, 이 안전망이 따로 잡아 주는 대상은 사실상 없고 부작용만 남는다.
- **제안**: 부착 목록 순회를 없애고 추적 인스턴스만 정리한다. 보완 탐색을 꼭 남겨야 한다면 최소한 `IWxSpawnable` 구현 여부로 대상을 좁힌다(배치 적까지 걸리므로 완전한 해법은 아니다). 빙의 시 Owner 가 컨트롤러로 바뀌므로(`:154`) `GetOwner() == this` 검사는 쓸 수 없다. 이 제안은 적의 스포너 부착(확정 설계)을 건드리지 않는다.
- **확신도**: 높음 — 다만 현재 `__ExternalActors__` 검색에서는 스포너에 부착된 배치 액터가 확인되지 않아, 지금은 저작 전에 막을 잠재 결함이다.

### 3. 🟡 `bEndingPlay` 를 되돌리지 않아, 월드에서 제거됐다 다시 추가된 장치는 동기화가 영구히 멈춘다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:78-83`, `:232-235`, `:143`, `:63-76`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:123`
- **범주**: 버그/정확성
- **문제**: `EndPlay` 가 세운 `bEndingPlay`(`:80`)를 어디서도 false 로 되돌리지 않는다. 레벨이 언로드 없이 월드에서만 빠지면(`EEndPlayReason::RemovedFromWorld`) 엔진은 액터 인스턴스를 남긴 채 초기화만 해제하고(엔진 `Actor.cpp:3234-3243`, 이어서 `UninitializeComponents`), 다시 추가될 때 같은 인스턴스에 `BeginPlay` 를 다시 보낸다(엔진 `Level.cpp:3909`). 스트리밍 레벨을 "로드 유지·비가시"로 내리거나 WP 셀이 Activated→Loaded 로 내려가는 경우(스트리밍 소스 `TargetState=Loaded`, 런타임 데이터 레이어 Loaded 등)가 여기에 해당한다.

  재활성화된 장치에서는 `BeginPlay`(`:63-76`) → `StartLogic` → `Synchronize` 가 `:232-235` 에서 곧바로 return 한다.
  - 권위: 새로 시작한 트리를 발행하지 못하고 `StopLogic` 발행(`:143`)도 막혀, 클라이언트는 제거 전의 옛 스냅샷을 계속 따른다. `InitialState` 를 지정한 장치는 `InitialTarget` 이 영구히 남아 1번 (b) 와 같은 "모든 전이가 복원" 고착이 싱글플레이에서도 생긴다.
  - 클라이언트: 재활성화된 인스턴스에서 `FollowAuthorityState` 가 돌지 않고, `OnRep_StateSnapshot` 경로도 같은 return 에 막혀 권위 상태를 전혀 따라가지 않는다.

  두 경우 모두 `SyncFailure` 를 거치지 않아 에러 로그가 남지 않는다.
- **제안**: 플래그가 필요한 구간은 `Super::EndPlay` 가 부르는 `StopLogic` 뿐이다(엔진 `StateTreeComponent.cpp:107`). 멤버 플래그를 오래 들고 있지 말고 `TGuardValue<bool>` 로 `Super::EndPlay` 호출 구간만 감싸거나, 최소한 `BeginPlay` 에서 되돌린다.
- **확신도**: 중간 — 코드 경로와 엔진의 재 `BeginPlay` 는 확인했다. 실제 발생은 레벨 스트리밍·WP 스트리밍 구성에 달려 있다.

### 4. 🟡 링크 에셋의 태그를 스냅샷에 싣지만 추종은 루트 에셋에서만 한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196-221`, `:440-444`, `:346-350`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState` 는 활성 프레임 전체를 역순으로 훑으므로, `Frame.StateTree` 가 루트가 아닌 링크 에셋(에셋 링크·`LinkedStateTreeOverrides`) 프레임의 태그 상태도 `LastEnteredTag` 로 채택되고(`:213`) 권위가 그 태그를 발행한다. 반면 `HasState` 와 `RequestState` 는 `StateTreeRef.GetStateTree()`(루트 에셋)에만 묻는다(`:440-444`, `:417`, `:427`). 루트에 없는 태그가 실리면 클라이언트는 `:346-350` 에서 `FailSynchronization` 을 찍고 그 장치의 동기화를 멈춘다. `SyncFailure` 는 스냅샷의 serial·tag·RunStatus 가 바뀔 때만 풀리므로(`:291-302`) 같은 상태에 머무는 동안에는 복구되지 않는다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:53`)의 "태그는 루트 에셋에서 유일한 상태 식별자" 계약을 관측 코드가 강제하지 않는 것이 원인이다.
- **제안**: 관측 범위를 루트 에셋 프레임(`Frame.StateTree == StateTreeRef.GetStateTree()`)으로 한정한다. 링크 에셋 태그까지 식별해야 한다면 상태 식별·전이 요청에 프레임(에셋) 문맥을 함께 넣는다.
- **확신도**: 중간 — 장치 ST 가 태그 상태를 가진 링크 에셋을 쓸 때만 드러난다.

### 5. 🟡 스포너 발동 태스크만 장치 복원 판정을 쓰지 않아 `InitialState` 복구에서 스폰한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: 이 태스크만 `!Transition.SourceStateID.IsValid()` 하나로 초기 진입을 판정한다. 같은 모듈의 다른 일회성 태스크는 모두 `FWxDeviceExecutionPolicy::IsRestoring`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp:8-11`)으로 장치의 `bRestoringState` 도 함께 본다. `InitialState` 를 지정한 장치는 시작 직후 `Synchronize` 가 `RequestState(InitialTarget)`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:249`)로 전이를 거는데, 이 전이는 `SourceStateID` 가 유효해 이 태스크에는 라이브 발동으로 보인다. 클라이언트의 복원 재요청 전이도 마찬가지다(권위 게이트 `:31` 가 막아 증상은 권위에만 난다).

  그 결과 레벨 시작 시 권위에서 `Respawn()` 이 돈다. `Respawn()` 은 `SpawnMode` 를 보지 않으므로(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:51-68`) 트리거를 기다려야 할 Manual 스포너까지 시작부터 적을 스폰한다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawners.h:27`)의 "초기 진입이면 호출하지 않는다"는 약속과도 어긋난다.
- **제안**: 다른 일회성 태스크처럼 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 으로 통일한다. 오너가 장치가 아니면(퀘스트 ST) 기존 판정과 똑같이 동작한다.
- **확신도**: 높음 — 현재 이 태스크는 `Content/Quest/Steps/ST_QuestStep_KillEnemies.uasset` 에서만 쓰여 증상이 없고, 장치 ST 에 넣는 순간 드러난다.

### 6. 🟡 상태 도중 당사자가 바뀌거나 사라지면 몽타주 대기가 남의 몽타주를 보거나 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:42-47`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:43`, `:72`
- **범주**: 버그/정확성
- **문제**: 틱은 재생을 건 캐릭터가 아니라 장치의 `InteractingCharacter` 를 매번 다시 읽는다(`:42-43`). 그런데 그 값은 신호가 전이로 이어지는지와 상관없이 먼저 덮어써진다. `OnInteracted` 는 발행 결과를 보기 전에 대입하고(`WxDevice.cpp:43`), `NotifyDeviceInteracted` 는 이벤트를 듣는 전이가 없어도 null 까지 그대로 대입한다(`:72`).

  부모 상태가 켠 상호작용은 자식 상태로 이어지므로(`Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h:69`), 몽타주 상태 도중 다른 플레이어가 누르면 권위 쪽 태스크는 그 사람의 AnimInstance 를 폴링하다 곧바로 Succeeded 로 상태를 끊는다. 당사자 없는 발신자가 `SendEvent` 를 보내 값이 null 이 되면 `:44-47` 에서 Failed 가 된다. 클라이언트에서는 `ApplyInteractor`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:316-326`)가 매 틱 무효 당사자를 null 로 바꾸므로, 당사자가 사망·파괴되는 순간에도 Failed 로 끝난다.

  진입 시 대상 부재(`:28-31`)와 틱 중 AnimInstance 부재(`:51-54`)는 Succeeded 이고 헤더(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:28-29`)도 "갇히지 않게 완료"를 약속하는데, 이 분기만 Failed 라서 성공 전이만 저작한 장치는 다음 상태로 넘어가지 못한다.
- **제안**: 진입 시 재생을 건 캐릭터를 인스턴스 데이터에 약참조로 담고 그것만 폴링한다. 무효 판정은 `IsValid` 로 하고 결과는 Succeeded 로 통일한다. `NotifyDeviceInteracted` 가 당사자를 null 로 덮지 않게 하는 것도 함께 검토한다.
- **확신도**: 중간 — 코드 경로는 확실하다. 현재 이 태스크를 쓰는 에셋은 없어 저작 전에 고칠 잠재 결함이다.

### 7. 🟡 지속 연출용 사운드·Niagara 태스크가 상태를 떠나도 멈추지 않고, 재진입·재시작마다 겹친다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlaySound.h:24-26`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp:36-39`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25-28`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SpawnNiagara.h:38-40`
- **범주**: 버그/정확성
- **문제**:
  - (a) `PlaySound` 헤더는 `bPlayOnRestore = true` 를 "상태에 묶인 지속 사운드용"이라고 안내하지만, 구현은 핸들을 돌려주지 않는 `UGameplayStatics::PlaySoundAtLocation`(`:38`)이고 `ExitState` 도 없다. 루프 사운드를 넣으면 상태를 떠나도 멈추지 않고, 복원 진입이 반복될 때마다 인스턴스가 겹친다.
  - (b) `SpawnNiagara` 는 "이 노드가 띄운 Niagara 가 재생 중이면 다시 띄우지 않는다"는 판정을 인스턴스 데이터의 `SpawnedComponent`(`:25-28`)에 기댄다. 그런데 엔진은 새로 진입하는 상태의 태스크 인스턴스 데이터를 템플릿에서 새로 만들고 공통이 아닌 기존 데이터는 버린다(엔진 `StateTreeExecutionContext.cpp:2677-2684`, `:2716-2722`). 그래서 이 판정은 같은 상태의 재선택에서만 통하고, 상태를 떠났다 돌아오거나 트리를 재시작하면 루프 FX 가 하나씩 더 쌓이며 옛 컴포넌트는 누구도 멈추지 않는다.

  재시작 경로로는 클라이언트 재동기화의 `Super::RestartLogic`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:402-409`)과 1번의 복원 반복이 있다.
- **제안**: 지속 연출 경로에서는 스폰한 컴포넌트(`SpawnSoundAtLocation`·Niagara 컴포넌트)를 인스턴스 데이터에 두고 `ExitState` 에서 멈춘다. 상태 이후에도 FX 를 남기는 것이 의도라면 컴포넌트를 장치 쪽에 보관해 중복 판정이 인스턴스 데이터 수명에 기대지 않게 한다. 일회성 전용으로 둘 거라면 헤더 안내를 고친다.
- **확신도**: 중간 — `SpawnNiagara` 는 `ST_CheckPoint` 에서 쓰이며, (b) 는 체크포인트 트리의 재시작·상태 이탈 후 복귀 경로에서 드러난다. `PlaySound` 를 쓰는 에셋은 현재 없다.

### 8. 🟡 입력 차단 태스크가 HUD 상호작용을 막지 못하고 중첩 차단도 세지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:43-53`, `:58-70`
- **범주**: 설계/구조
- **문제**:
  - (a) `APawn::DisableInput` 은 폰의 `bInputEnabled` 하나만 끈다. 그런데 상호작용 요청은 HUD 위젯 → `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:85` → 스캐너 `TryInteractSelected`(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:61-70`) → `ServerInteract` 로 흘러 폰 입력을 거치지 않는다. 실질 게이트인 상호작용 어빌리티의 차단 태그(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:30-36`)에도 이 태스크는 아무것도 걸지 않는다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_EnablePlayerInput.h:34`)의 "입력 전체"를 믿고 연출 차단에 쓰면, 그 사이 다른 장치와 상호작용할 수 있다.
  - (b) `APawn::EnableInput/DisableInput` 은 불리언 대입이라 횟수를 세지 않는데(엔진 `Pawn.cpp:1165-1187`), `ExitState` 는 무조건 되돌린다. (a) 때문에 연출 중인 장치 A 와 새로 발동한 장치 B 가 같은 폰의 입력을 차례로 끄는 조합이 가능하고, B 가 먼저 끝나면 A 의 연출 도중에 입력이 돌아온다.

  같은 모듈은 상호작용 바인딩에 토큰 스택(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:90-120`)을 두어 같은 문제를 이미 풀어 두었는데, 이 태스크만 그 방식을 따르지 않는다.
- **제안**: 조작 전체를 막는 것이 목적이라면, 서버 어빌리티와 로컬 스캐너가 함께 보는 차단 상태(권위에서 부여하는 차단 태그 등)를 태스크 수명 동안 유지하고 중첩 계산도 그 태그에 맡긴다. 폰 입력만 끄는 것이 의도라면 헤더 표현을 좁히고 차단 주체를 토큰으로 센다.
- **확신도**: 중간 — 현재 이 태스크를 쓰는 에셋은 없다.

### 9. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h`(GameplayStateTreeModule)를 포함하는데, 그 모듈은 Private 의존성이다. 클래스도 export 되지 않아(`:56-57`, 근거 `Source/WxEditor/WxDeviceLinkVisualizer.h:12`) 모듈 밖에서는 쓸 수 없다. Public 배치로 얻는 것은 포함 경로가 보장되지 않는 헤더를 외부에 노출하는 것뿐이다. export 없는 `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceExecutionPolicy.h` 도 같은 경우다(현재 두 헤더 모두 모듈 밖 포함 0건).
- **제안**: 외부에서 쓸 계획이 없다면 두 헤더를 Private 로 옮겨 배치와 의도를 맞춘다. 외부에 열 계획이 있다면 `GameplayStateTreeModule` 을 Public 의존성으로 올리고 export 한다.
- **확신도**: 중간

### 10. 🟢 스플라인 이동 헤더가 "진입 경로를 가리지 않고 슬라이드한다"고 적지만 복원 진입에서는 스냅한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SplineMove.h:59`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp:50`
- **범주**: 설계/구조
- **문제**: 구조체 설명은 진입 경로와 무관하게 현재 위치에서 목표까지 슬라이드한다고 약속하지만, 구현은 `FWxDeviceExecutionPolicy::IsRestoringDevice` 가 참이면 목표로 곧바로 스냅한다(`:50`). 같은 헤더의 필드 주석(`:29`, "초기 진입 스냅·라이브 슬라이드")과도 서로 어긋난다. README 가 "복원/라이브 구분을 먼저 결정하라"를 이 모듈의 핵심 규약으로 두고 있어, 기존 태스크의 복원 정책을 잘못 적은 설명은 새 태스크 작성자를 오도한다.
- **제안**: 구조체 설명을 "복원 진입은 목표로 스냅하고, 라이브 진입은 현재 위치에서 슬라이드한다"로 고친다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Tests/WxDeviceInteractorSyncTest.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 태스크·시스템 파일(`WxStateTreeTask_PlayAnimation`, `WxStateTreeTask_ApplyGameplayEffectToInteractor`, `WxStateTreeTask_RecordCheckpoint`, `WxStateTreeTask_RespawnSpawners`, `WxSpawnable`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxCheckpointSubsystem`, `WxWorldDeveloperSettings`, `WxWorldModule`)과 대응 헤더 전부.
  - 소비 측 대조: `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxEditor/WxDeviceLinkVisualizer.h`, `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`.
  - 엔진 동작 대조(UE 5.8.1, `Engine/Build/Build.version`): `StateTreeExecutionContext.cpp`(`TickTriggerTransitionsInternal`, `Start` 의 선행 `Stop`, 상태 인스턴스 데이터 할당·`ShrinkTo`), `StateTreeAsyncExecutionContext.cpp`(`FinishTask` 는 상태만 세우고 전이를 즉시 적용하지 않음 — 등록부 주석과 일치), `StateTreeComponent.cpp`(`BeginPlay` 의 `StartLogic`, `EndPlay` 의 `StopLogic`, 순정 실행 확장), `Actor.cpp`(`RouteEndPlay` 의 RemovedFromWorld 처리), `Level.cpp`(재 `DispatchBeginPlay`), `Pawn.cpp`(`EnableInput/DisableInput`), `LevelSequenceActor.cpp`·`LevelSequencePlayer.cpp`(권위 생성 시퀀스 액터는 `bReplicatePlayback=false` 로 복제가 꺼져 피어별 로컬 재생과 충돌하지 않음).
  - 콘텐츠 대조(에셋 문자열 검색): `SendEvent`·`EnableInteraction`·`ComponentMove` 등은 장치 ST 에서 쓰인다. `SpawnNiagara`·`RespawnSpawners`·`RecordCheckpoint` 는 `ST_CheckPoint`, `TriggerSpawners`·`WaitSpawnersKilled` 는 `ST_QuestStep_KillEnemies`, `WaitForInteraction` 은 `ST_QuestStep_InteractTarget` 에서 쓰인다. `PlaySound`·`PlayInteractorMontage`·`EnablePlayerInput`·`PlayLevelSequence` 를 쓰는 에셋은 없다.
  - 규칙 점검(58파일 전부): Copyright 첫 줄은 모두 있다. `FORCEINLINE`·`inline` 은 없고, 헤더 본문 정의는 `GetInstanceDataType()` 16곳과 템플릿 `TWxStateTreeWaitRegistry` 뿐이며 모두 예외 사유 주석이 있다. 모듈 밖 Wx 헤더 포함은 `WxCore` 의 `WxInteractable.h`·`WxGameplayTags.h`·`WxLocatorUtils.h` 셋뿐이고, `.uplugin` 의 Wx 의존도 `WxCore` 하나다.
- **미검토 / 한계**:
  - 이번 검토는 C++ 정적 검토다. 빌드, PIE 재현, 자동화 테스트 실행은 하지 않았고, StateTree/BP/WBP 에셋 내부 구조는 범위 밖이다(Unreal MCP 서버 연결 실패로 에셋 덤프도 못 했다). 1번의 `ST_Piston` 상태 구성, 2번의 부착 배치 여부는 에셋 문자열 검색까지만 확인했다.
  - 3번은 레벨 스트리밍·WP 스트리밍 구성에, 4번은 링크 에셋 저작에, 5·6·8번과 7번 (a) 는 해당 태스크의 향후 저작에 따라 체감이 달라진다.
  - 다음 항목은 의도된 설계나 낮은 위험으로 보아 싣지 않았다.
    - 클라이언트 로컬 완료 전이로 다른 태그 상태에 먼저 들어가면 `FollowAuthorityState` 가 복제 태그로 되감는 것: "클라 전이는 복제 State 가 게이트" 확정 설계다.
    - `IsAwaited`(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:36`)의 `Target` 무검사 역참조: 호출부가 `AWxNpc::CanInteract` 의 `this` 하나뿐이다. 등록부가 권위 월드에만 있어 원격 클라이언트 스캐너에서는 NPC 가 후보가 되지 않는데, 이는 `Source/WxGame/Character/WxNpc.cpp:42` 가 명시한 "서버가 곧 클라" 전제이며 WxGame 소관이다.
    - `WxStateTreeTask_RecordCheckpoint.cpp:43` 의 서브시스템 무검사 역참조: 항상 생성되는 GameInstance 서브시스템이다. 이 태스크만 `GetDescription` 이 없는 것은 README 관례일 뿐 CLAUDE.md 규칙이 아니다.
    - `AWxDevice::BeginPlay` 가 부모 장치를 암묵적으로 링크하는 것(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:126-132`), `PlayAnimation` 의 복원 재생(헤더 명시), `PlayLevelSequence` 의 복원 침묵(헤더 명시).
    - `TWxStateTreeWaitRegistry` 안의 중첩 구조체 `FWait` 에 `Wx` 접두사가 없는 것: 템플릿 스코프 안이라 이름 충돌 위험이 없어 규칙 위반으로 보지 않았다.
    - `AWxDevice::SetInteractionBinding` 이 public 이라 토큰 스택을 우회할 수 있는 것: 현재 호출부가 Push/Pop 내부뿐이다.
    - 자동화 테스트의 `friend class FWxDeviceInteractorSyncTest`(헤더 `:91-93`)와 `/Game/WorldObject/Gimmick/BP_Door` 콘텐츠 결합: 테스트 관례 수준이다. 문 에셋의 태그(`Device.Door.Open`/`Close`)·구조가 바뀌면 함께 깨진다.
  - 장치 액터가 dormancy 없이 복제되는 서버 비용, 클라이언트 추종 틱마다의 `RequestGameplayTag`·루트 에셋 상태 탐색 비용, 스캐너의 `AllObjects` 오버랩(반경 150cm, 0.1초 주기) 비용은 측정하지 않았다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 58파일 — `/module-review`로 갱신*
