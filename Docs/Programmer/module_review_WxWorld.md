# WxWorld — 코드 리뷰

> 모듈 경계(`WxCore` 외 Wx 플러그인 무참조)와 CLAUDE.md 코딩 규칙은 58파일 전부에서 깨끗하다. 직전 리뷰의 🔴(미해소 당사자 참조로 원격 클라이언트 추종이 멈춤)는 `6c2cb5837` 에서 고쳐졌음을 현재 코드로 확인해 뺐다. 남은 결함은 장치 동기화 컴포넌트의 "초기 상태 판정"과 "EndPlay 플래그 수명", 스포너 정리 범위, 그리고 연출 태스크 몇 개의 복원·수명 규약 불일치다. 대부분 특정 저작이나 스트리밍 조건에서 드러나는 잠재 결함이다.
> 커버리지: 장치 동기화·상호작용 표면·스포너·연출 태스크는 cpp 까지 깊게 봤고, 나머지 시스템·설정 파일은 훑었다. 기존 발견은 모두 현재 코드와 UE 5.8.2 엔진 소스로 다시 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 8 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 즉시 완료되는 태그 상태를 `InitialState` 로 지정하면 권위 장치가 발행과 일회성 실행을 계속 멈춘다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:239-253`, `:151`, `:143`, `:393-397`, `:352`
- **범주**: 버그/정확성
- **문제**: 초기 상태 도달 판정은 틱이 끝난 시점에 `LastEnteredTag == InitialTarget` 인지 하나만 본다(`:241`). 그런데 엔진은 `EnterState` 가 완료를 반환하면 같은 틱 안에서 다음 전이를 최대 5회까지 이어서 적용한다(엔진 `StateTreeExecutionContext.cpp:1980-2031`). 따라서 복원 진입에서 곧바로 끝나 완료 전이로 다른 태그 상태로 넘어가는 태그 상태를 `InitialState` 로 지정하면, 대상 진입을 실제로 거쳤는데도 매번 불일치로 판정된다. 요청 세 번 뒤 `FailSynchronization`(`:393-397`)이 걸리지만 `InitialTarget` 은 비워지지 않으므로 증상이 풀리지 않는다.
  - (a) `Synchronize` 가 매 틱 `:249-250` 에서 return 해 `PublishAuthorityState`(`:253`)에 닿지 못한다. `StopLogic` 발행(`:143`)도 같은 조건으로 막혀 클라이언트는 이 장치를 한 번도 따라가지 못한다.
  - (b) `TickComponent` 가 매 틱 `bRestoringState` 를 참으로 세운다(`:151`). 이후 모든 라이브 전이가 복원으로 취급되어 `SendEvent`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners` 가 권위에서 영영 실행되지 않고, 이동은 슬라이드 대신 스냅한다. 싱글플레이에서도 똑같다.

  클라이언트의 요청 성공 판정(`:352`)도 같은 방식이지만, 새 스냅샷이 오면 `SyncFailure` 가 풀려 영향이 제한된다. 에디터 드롭다운(`:464-480`)은 이런 상태를 거르지 않고 모든 태그 상태를 후보로 보여 준다.
- **제안**: 성공 판정을 "요청 이후 대상 태그 진입을 관측했는가"로 바꾼다. 전이 관측 지점(`HandleBeginApplyTransition`/`ObserveActiveState`)에서 대상 태그 진입을 기록하면 된다. 권위 쪽은 초기 상태 요청이 실패로 끝나면 `InitialTarget` 을 비워 발행·라이브 실행으로 돌아가게 한다.
- **확신도**: 높음 — 코드와 엔진 전이 루프로 확인했다. 실제 발생 여부는 전이형 태그 상태를 `InitialState` 로 쓰는 저작이 있는지에 달려 있다.

### 2. 🟡 스포너 정리가 무관한 부착 액터까지 파괴하고, 체크포인트 휴식이 이를 월드 전체로 넓힌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:171-180`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:23-36`
- **범주**: 버그/정확성
- **문제**: `DestroySpawnedActor` 는 추적하던 `SpawnedActor` 를 파괴한 뒤, 스포너에 직접 부착된 액터를 종류나 생성 주체와 상관없이 전부 `Destroy()` 한다. 이 경로는 `Respawn()`(`:59`)과 `EndPlay()`(`:102`) 양쪽에서 돌고, `TryRespawnAll` 은 Manual 이 아닌 월드의 모든 스포너에 `Respawn()` 을 부른다. 그래서 체크포인트 휴식(`RespawnSpawners` 태스크) 한 번에 스포너에 붙여 둔 배치 액터가 레벨 곳곳에서 함께 사라지고, 레벨을 다시 읽기 전까지 돌아오지 않는다. 정찰 경로 규약은 직접 배치한 적도 경로를 그린 액터에 부착해 정찰하게 허용한다. 따라서 스포너의 경로를 공유하려고 그 스포너에 부착해 둔 배치 적이 대표적인 피해 대상이다.

  주석(`:171`)이 근거로 드는 "스스로 부착하는 적"은 `AWxEnemyCharacter::OnSpawnedBy`(`Source/WxGame/Character/WxEnemyCharacter.cpp:94`)다. 그런데 그 인스턴스는 바로 뒤에 `SpawnTarget()` 이 `SpawnedActor` 에 담는 같은 객체다(`:151`). 약참조가 무효해지는 경우는 액터가 파괴된 경우뿐이고, 파괴된 액터는 부착에서도 풀린다. 이 안전망이 따로 잡아 주는 대상은 사실상 없고 부작용만 남는다.
- **제안**: 부착 목록 순회를 없애고 추적 인스턴스만 정리한다. 보완 탐색을 꼭 남겨야 한다면 최소한 `IWxSpawnable` 구현 여부로 대상을 좁힌다(배치 적까지 걸리므로 완전한 해법은 아니다). `GetOwner() == this` 검사는 빙의 시 Owner 가 컨트롤러로 바뀌므로(`:154`) 쓸 수 없다. 이 제안은 적의 스포너 부착(확정 설계)을 건드리지 않는다.
- **확신도**: 높음

### 3. 🟡 스포너 발동 태스크만 장치 복원 판정을 쓰지 않아 `InitialState` 복구에서 스폰한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: 이 태스크만 `!Transition.SourceStateID.IsValid()` 하나로 초기 진입을 판정한다. 같은 모듈의 다른 일회성 태스크는 모두 `FWxDeviceExecutionPolicy::IsRestoring`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp:8-11`)으로 장치의 `bRestoringState` 도 함께 본다. `InitialState` 를 지정한 장치는 시작 직후 `Synchronize` 가 `RequestState(InitialTarget)`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:249`)으로 전이를 건다. 이 전이는 `SourceStateID` 가 유효하므로 이 태스크에는 라이브 발동으로 보인다.

  그 결과 레벨 시작 시 권위에서 `Respawn()` 이 돈다. `Respawn()` 은 `SpawnMode` 를 보지 않으므로(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:51-68`) 트리거를 기다려야 할 Manual 스포너까지 시작부터 적을 스폰한다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawners.h:27`)의 "초기 진입이면 호출하지 않는다"는 약속과도 어긋난다.
- **제안**: 다른 일회성 태스크처럼 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 으로 통일한다. 오너가 장치가 아니면(퀘스트 ST) 기존 판정과 똑같이 동작한다.
- **확신도**: 높음 — 퀘스트 ST 에서는 증상이 없고, 장치 ST 에 이 태스크를 넣을 때 드러난다.

### 4. 🟡 `bEndingPlay` 를 되돌리지 않아, 월드에서 제거됐다 다시 추가된 장치는 동기화가 영구히 멈춘다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:78-83`, `:232-235`, `:143`, `:63-76`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:123`
- **범주**: 버그/정확성
- **문제**: `EndPlay` 가 세운 `bEndingPlay`(`:80`)를 어디서도 false 로 되돌리지 않는다. 엔진은 레벨이 언로드 없이 월드에서만 제거되면(`EEndPlayReason::RemovedFromWorld`) 액터 인스턴스를 그대로 둔 채 초기화만 해제한다(엔진 `Actor.cpp:3234-3243`). 그리고 다시 추가될 때 같은 인스턴스에 `BeginPlay` 를 다시 발송한다(엔진 `Level.cpp:3909`). WP 에서 셀이 Activated→Loaded 로 내려가는 경로(엔진 `WorldPartitionStreamingPolicy.cpp:988`, 스트리밍 소스 `TargetState=Loaded`·런타임 데이터 레이어 Loaded 등)가 이 경우다.

  재활성화된 장치에서는 `BeginPlay`(`:63-76`) → `StartLogic` → `Synchronize` 가 `:232-235` 에서 곧바로 return 한다.
  - 권위: 새로 시작한 트리를 발행하지 못하고, `StopLogic` 발행(`:143`)도 같은 플래그에 막힌다. 그래서 클라이언트는 제거 전의 옛 스냅샷을 계속 따른다. `InitialState` 를 지정한 장치는 `InitialTarget` 이 영구히 남아 1번 (b) 와 같은 "모든 전이가 복원" 고착이 싱글플레이에서도 생긴다.
  - 클라이언트: 인스턴스가 재활성화되면 `FollowAuthorityState` 가 돌지 않아 권위 상태를 전혀 따라가지 않는다. `OnRep_StateSnapshot` 경로도 같은 return 에 막힌다.

  두 경우 모두 `SyncFailure` 를 거치지 않아 에러 로그가 남지 않는다.
- **제안**: 플래그가 필요한 구간은 `Super::EndPlay` 가 부르는 `StopLogic` 뿐이다. 멤버 상태를 오래 들고 있지 말고 `TGuardValue<bool>` 로 `Super::EndPlay` 호출 구간만 감싸거나, 최소한 `BeginPlay` 에서 되돌린다.
- **확신도**: 중간 — 코드 경로와 엔진의 재 `BeginPlay` 는 확인했다. 현재 C++ 에는 데이터 레이어·레벨 가시성·스트리밍 소스 상태를 조작하는 코드가 없어(검색 0건), 실제 발생은 콘텐츠와 WP 스트리밍 설정에 달려 있다.

### 5. 🟡 링크 에셋의 태그를 스냅샷에 싣지만 추종은 루트 에셋에서만 한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196-221`, `:440-444`, `:346-350`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState` 는 활성 프레임 전체를 역순으로 훑는다. 그래서 `Frame.StateTree` 가 루트가 아닌 링크 에셋 프레임의 태그 상태도 `LastEnteredTag` 로 채택되고(`:213`), 권위가 그 태그를 발행한다. 반면 `HasState` 와 `RequestState` 는 `StateTreeRef.GetStateTree()`(루트 에셋)에만 묻는다(`:440-444`, `:417`, `:427`). 루트에 없는 태그가 스냅샷에 실리면 클라이언트는 `:346-350` 에서 `FailSynchronization` 을 찍고 그 장치의 동기화를 멈춘다. `SyncFailure` 는 스냅샷의 serial·tag·RunStatus 가 바뀔 때만 풀리므로(`:291-302`), 같은 상태에 머무는 동안에는 복구되지 않는다. 원인은 관측 코드가 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:53`)의 "태그는 루트 에셋에서 유일한 상태 식별자" 계약을 지키지 않는 데 있다.
- **제안**: 관측 범위를 루트 에셋 프레임(`Frame.StateTree == StateTreeRef.GetStateTree()`)으로 한정한다. 또는 상태 식별·전이 요청에 프레임(에셋) 문맥까지 넣어 링크 에셋 태그도 되짚을 수 있게 한다.
- **확신도**: 중간 — 장치 ST 가 태그 상태를 가진 링크 에셋을 쓸 때만 드러난다.

### 6. 🟡 상태 도중 당사자가 바뀌거나 사라지면 몽타주 대기가 남의 몽타주를 보거나 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:42-47`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:43`, `:72`
- **범주**: 버그/정확성
- **문제**: 틱은 재생을 건 캐릭터가 아니라 장치의 `InteractingCharacter` 를 매번 다시 읽는다(`:42-43`). 그런데 그 값은 신호가 전이로 이어지는지와 상관없이 먼저 덮어써진다. `OnInteracted` 는 발행 결과를 보기 전에 대입하고(`WxDevice.cpp:43`), `NotifyDeviceInteracted` 는 이벤트를 듣는 전이가 없어도 null 까지 그대로 대입한다(`:72`).

  부모 상태가 켠 상호작용은 자식 상태로 이어진다(`Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h:69`). 그래서 몽타주 상태 도중 다른 플레이어가 누르면, 권위 쪽 태스크는 그 사람의 AnimInstance 를 폴링하다 곧바로 Succeeded 로 상태를 끊는다. 당사자 없는 발신자가 `SendEvent` 를 보내 값이 null 이 되면 `:44-47` 에서 Failed 가 된다. 클라이언트에서는 `6c2cb5837` 이후 `ApplyInteractor`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:316-326`)가 매 틱 무효 당사자를 null 로 바꾸므로, 당사자가 사망·파괴되는 순간에도 Failed 로 끝난다.

  진입 시 대상 부재(`:28-31`)와 틱 중 AnimInstance 부재(`:51-54`)는 Succeeded 이고, 헤더(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:28-29`)도 "갇히지 않게 완료"를 약속한다. 이 분기만 Failed 라서, 성공 전이만 저작한 장치는 다음 상태로 넘어가지 못한다. 반대로 권위에서는 `TObjectPtr` 가 GC 전까지 파괴된 포인터를 쥐고 있어, 헤더가 폴링의 근거로 드는 사망·리스폰을 `!Character` 로 잡지 못한다.
- **제안**: 진입 시 재생을 건 캐릭터를 인스턴스 데이터에 약참조로 담고 그것만 폴링한다. 무효 판정은 `IsValid` 로 하고 결과는 Succeeded 로 통일한다. `NotifyDeviceInteracted` 가 당사자를 null 로 덮지 않게 하는 것도 함께 검토한다.
- **확신도**: 중간 — 코드 경로는 확실하다. 실제 영향은 몽타주 상태에서 상호작용을 열어 두거나, 당사자 없는 발동 장치를 연결한 저작이 있는지에 달려 있다.

### 7. 🟡 지속 연출용 사운드·Niagara 태스크가 상태를 떠나도 멈추지 않고, 재진입·재시작마다 겹친다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlaySound.h:24-26`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp:36-39`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25-28`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SpawnNiagara.h:38-40`
- **범주**: 버그/정확성
- **문제**:
  - (a) `PlaySound` 헤더는 `bPlayOnRestore = true` 를 "상태에 묶인 지속 사운드용"이라고 안내한다. 하지만 구현은 핸들을 돌려주지 않는 `UGameplayStatics::PlaySoundAtLocation`(`:38`)이고 `ExitState` 도 없다. 루프 사운드를 넣으면 상태를 떠나도 멈추지 않고, 복원 진입이 반복될 때마다 인스턴스가 겹친다.
  - (b) `SpawnNiagara` 는 "이 노드가 띄운 Niagara 가 재생 중이면 다시 띄우지 않는다"는 판정을 인스턴스 데이터의 `SpawnedComponent`(`:25-28`)에 기댄다. 그런데 엔진은 새로 진입하는 상태의 태스크 인스턴스 데이터를 템플릿에서 새로 만들고, 공통이 아닌 기존 데이터는 버린다(엔진 `StateTreeExecutionContext.cpp:2677-2684`, `:2716-2722`). 그래서 이 판정은 같은 상태의 재선택에서만 통한다. 상태를 떠났다 돌아오거나 트리를 재시작하면 루프 FX 가 하나씩 더 쌓이고, 옛 컴포넌트는 누구도 멈추지 않는다.

  재시작 경로로는 클라이언트 재동기화의 `Super::RestartLogic`(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:402-409`)과 1번의 복원 반복이 있다.
- **제안**: 두 태스크 모두 지속 연출 경로에서는 스폰한 컴포넌트(`SpawnSoundAtLocation`·Niagara 컴포넌트)를 인스턴스 데이터에 두고 `ExitState` 에서 멈춘다. 상태 이후에도 FX 를 남기는 것이 의도라면, 컴포넌트를 장치 쪽에 보관해 중복 판정이 인스턴스 데이터 수명에 기대지 않게 한다. 일회성 전용으로 둘 거라면 헤더 안내를 고친다.
- **확신도**: 중간 — 구현 한계와 엔진 인스턴스 데이터 수명은 확인했다. 실제 영향은 루프 에셋을 쓰는지와 그 상태를 떠나거나 재시작하는 흐름에 달려 있다.

### 8. 🟡 입력 차단 태스크가 HUD 상호작용을 막지 못하고 중첩 차단도 세지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:43-53`, `:58-70`
- **범주**: 설계/구조
- **문제**:
  - (a) `APawn::DisableInput` 은 폰의 `bInputEnabled` 하나만 끈다. 그런데 상호작용 요청은 HUD 위젯 → `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:85` → 스캐너 `TryInteractSelected`(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:61-70`) → `ServerInteract` 로 흘러 폰 입력을 거치지 않는다. 실질 게이트인 상호작용 어빌리티의 차단 태그(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:30-36`)에도 이 태스크는 아무것도 걸지 않는다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_EnablePlayerInput.h:34`)의 "입력 전체"를 믿고 연출 차단에 쓰면, 그 사이 다른 장치와 상호작용할 수 있다.
  - (b) `APawn::EnableInput/DisableInput` 은 불리언 대입이라 횟수를 세지 않는데(엔진 `Pawn.cpp:1165-1187`), `ExitState` 는 무조건 되돌린다. (a) 때문에 연출 중인 장치 A 와 새로 발동한 장치 B 가 같은 폰의 입력을 차례로 끄는 조합이 가능하다. 이때 B 가 먼저 끝나면 A 의 연출 도중에 입력이 돌아온다.

  같은 모듈은 상호작용 바인딩에 토큰 스택(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:90-120`)을 두어 같은 문제를 이미 풀어 두었는데, 이 태스크만 그 방식을 따르지 않는다.
- **제안**: 조작 전체를 막는 것이 목적이라면, 서버 어빌리티와 로컬 스캐너가 함께 보는 차단 상태(권위에서 부여하는 차단 태그 등)를 태스크 수명 동안 유지하고 중첩 계산도 그 태그에 맡긴다. 폰 입력만 끄는 것이 의도라면 헤더 표현을 좁히고 차단 주체를 토큰으로 센다.
- **확신도**: 중간

### 9. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h`(GameplayStateTreeModule)를 포함하는데, 그 모듈은 Private 의존성이다. 클래스도 export 되지 않아(`:56-57`, 근거는 `Source/WxEditor/WxDeviceLinkVisualizer.h:12`) 모듈 밖에서는 쓸 수 없다. 결국 Public 배치로 얻는 것은 포함 경로가 보장되지 않는 헤더를 외부에 노출하는 것뿐이다. export 없는 `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceExecutionPolicy.h` 도 같은 경우다.
- **제안**: 외부에서 쓸 계획이 없다면 두 헤더를 Private 로 옮겨 배치와 의도를 맞춘다. 외부에 열 계획이 있다면 `GameplayStateTreeModule` 을 Public 의존성으로 올리고 export 한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Tests/WxDeviceInteractorSyncTest.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 태스크·시스템 파일(`WxStateTreeTask_PlayAnimation`, `WxStateTreeTask_ApplyGameplayEffectToInteractor`, `WxStateTreeTask_RecordCheckpoint`, `WxStateTreeTask_RespawnSpawners`, `WxSpawnable`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxCheckpointSubsystem`, `WxWorldDeveloperSettings`, `WxWorldModule`)과 대응 헤더 전부.
  - 직전 리뷰 이후 변경: `6c2cb5837` 한 건(`WxDeviceStateTreeComponent` 의 `bHasInteractor` 제거·`IsValid` 발행, 자동화 테스트 신설). 직전 🔴 의 두 경로(미해소 NetGUID 대기, 파괴 포인터를 "당사자 있음"으로 발행)가 모두 사라졌음을 확인했다.
  - 소비 측 대조: `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxEditor/WxDeviceLinkVisualizer.h`.
  - 엔진 동작 대조(UE 5.8.2): `StateTreeExecutionContext.cpp`(`TickTriggerTransitionsInternal`, 상태 인스턴스 데이터 할당·`ShrinkTo`), `StateTreeComponent.cpp`(`EndPlay` 가 `StopLogic` 호출), `Actor.cpp`(`RouteEndPlay` 의 RemovedFromWorld 처리), `Level.cpp`(`RouteActorInitialize` 재 `BeginPlay`), `World.cpp`(제거 후 재추가 플래그 리셋), `WorldPartitionStreamingPolicy.cpp`·`WorldPartitionRuntimeLevelStreamingCell.cpp`(셀 Deactivate), `Pawn.cpp`(`EnableInput/DisableInput`).
  - 규칙 점검(58파일 전부): Copyright 첫 줄은 모두 있다. `FORCEINLINE`·`inline` 과 헤더 본문 정의는 없다(`GetInstanceDataType()` 과 `TWxStateTreeWaitRegistry` 는 예외 사유 주석이 있다). 람다는 `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:182` 의 정렬 술어 하나다. override 의 `Super::` 호출 누락은 없다. 모듈 밖 Wx 헤더 포함은 `WxCore` 의 `WxInteractable.h`·`WxGameplayTags.h`·`WxLocatorUtils.h` 셋뿐이다. `BlueprintCallable` 은 BP Function Library 의 `TryRespawnAll` 하나다.
- **미검토 / 한계**:
  - 이번 검토는 C++ 정적 검토다. 빌드, PIE 재현, 신설 자동화 테스트 실행, StateTree/BP/WBP 에셋 검증은 하지 않았다.
  - 1·5번은 `InitialState`·링크 에셋 저작에, 4번은 WP 셀 비활성화·데이터 레이어 등 스트리밍 구성에, 2번은 스포너에 부착된 배치 액터 유무에, 6·7·8번은 해당 태스크의 실제 저작에 따라 체감이 달라진다. 소스·설정 검색으로는 `PlayInteractorMontage`·`EnablePlayerInput`·`PlaySound`·`TriggerSpawners` 를 쓰는 에셋 흔적이 확인되지 않았으므로, 이 항목들은 저작 전에 고칠 잠재 결함으로 보는 편이 맞다. `SpawnNiagara` 는 체크포인트 ST 덤프(`Saved/CheckpointTasksVerify.json`, 09-06)에 쓰인 흔적이 있다. 다만 같은 상태 재진입(재선택)에서는 인스턴스 데이터가 유지되므로, 7번 (b) 는 트리 재시작·상태 이탈 후 복귀 경로에서만 드러난다.
  - 다음 항목은 의도된 설계나 낮은 위험으로 보아 싣지 않았다.
    - `IsAwaited`(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:36`)의 `Target` 무검사 역참조: 호출부가 `this` 하나뿐이다.
    - `WxStateTreeTask_RecordCheckpoint.cpp:43` 의 서브시스템 무검사 역참조: 항상 생성되는 GameInstance 서브시스템이다.
    - `AWxDevice::BeginPlay` 가 부모 장치를 암묵적으로 링크하는 것(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:126-132`): 확정 설계다.
    - `PlayAnimation` 의 복원 재생: 헤더에 명시되어 있다.
    - 자동화 테스트의 `friend class FWxDeviceInteractorSyncTest`(헤더 `:91-93`)와 `/Game/WorldObject/Gimmick/BP_Door` 콘텐츠 결합: 테스트 관례 수준이라 규칙 위반으로 보지 않았다. 다만 문 에셋의 태그·구조가 바뀌면 이 테스트가 함께 깨진다.
    - Build.cs 의 쓰이지 않는 `AIModule` 의존성: 사소하다.
  - 미해소 참조가 뒤늦게 풀릴 때 이미 진입한 상태의 태스크(몽타주 등)가 당사자 없이 건너뛴 연출을 다시 틀지 않는 것은, 새 헤더 계약("선택적 실행 문맥")에 따른 의도로 보았다.
  - 장치 액터가 dormancy 없이 복제되는 서버 비용과, 스캐너의 `AllObjects` 오버랩(반경 150cm, 0.1초 주기) 비용은 측정하지 않았다.

---
*문서 기준 커밋 `4096004a4` · 리뷰일 2026-09-16 · 소스 58파일 — `/module-review`로 갱신*
