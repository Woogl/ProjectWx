# WxWorld — 코드 리뷰

> 모듈 경계(`WxCore` 외 Wx 플러그인 무참조)와 CLAUDE.md 코딩 규칙은 57파일 전부에서 깨끗하다. 다만 장치 상태 동기화 컴포넌트에는 "당사자 해소 대기"와 "초기 상태 성공 판정" 두 곳에 장치가 멈추는 경로가 남아 있고, 연출 태스크 몇 개가 상태 수명·복원 규약과 어긋난다. 직전 리뷰(`9d8cb2dd`) 이후 이 모듈의 C++ 변경은 없어서 기존 발견을 현재 코드와 UE 5.8.2 엔진 소스로 다시 확인했다. 확정 설계("세션 내 스트리밍 영속 없음")와 부딪히는 스포너 처치 상태 항목은 뺐고, `SpawnNiagara` 중복 스폰은 엔진 인스턴스 데이터 수명을 근거로 새로 확인해 7번에 합쳤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 7 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 해소되지 않는 당사자 참조가 원격 클라이언트의 장치 상태 추종을 다음 발행까지 막는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:340-344`, `:318-322`, `:282-284`, `:264-268`
- **범주**: 설계/구조
- **문제**: `FollowAuthorityState` 는 `StateSnapshot.bHasInteractor` 가 참인데 `Interactor` 가 무효이면 그냥 return 한다(`:340-344`). 주석은 참조 매핑이 끝나면 RepNotify 가 다시 불린다고 가정하지만, 그 재통지는 참조가 실제로 해소될 때만 온다. 해소되지 않는 경로는 두 가지다.
  - (a) 당사자 캐릭터가 그 클라이언트의 relevancy 밖이면 NetGUID 가 계속 미해소로 남는다. 문을 연 플레이어가 멀어진 뒤 다른 플레이어가 그 문에 처음 다가오는 순서가 바로 이 경우다.
  - (b) 당사자가 파괴되면(사망·접속 종료) 서버 GC 가 `StateSnapshot.Interactor` 를 null 로 비운다. 그런데 `bHasInteractor` 는 새 진입이나 RunStatus 변화가 있을 때만 다시 계산되므로(`:264-268`) 참으로 남는다. 결국 클라이언트는 "null + 당사자 있음"을 받아 멈추고, 이후 접속하는 클라이언트도 모두 같은 스냅샷을 받는다. 게다가 발행 시 판정이 `IsValid` 가 아니라 `Interactor != nullptr`(`:284`)라서, GC 전에 파괴된 포인터도 "당사자 있음"으로 실린다.

  두 경우 모두 클라이언트는 `RequestState` 까지 가지 못한다. 그래서 서버에선 열린 문이 그 클라이언트에선 닫힌 채로 남고(콜리전 불일치), 로컬 트리의 상호작용 바인딩도 옛 상태라 스캐너 프롬프트까지 틀린다. `SyncAttempts`·`SyncFailure` 를 거치지 않으므로 에러 로그도 없고, 흔적은 `DescribeSynchronization` 의 `waitingInteractor` 플래그(`:463`)뿐이다.
- **제안**: 당사자 해소를 상태 수렴의 전제조건에서 뗀다. 상태는 태그만으로 적용하고, 당사자는 해소되는 대로 뒤늦게 채운다(당사자를 읽는 태스크는 이미 당사자 없는 진입 경로를 갖고 있다). 최소 조치는 두 가지다. `:284` 를 `IsValid` 판정으로 바꾸고, 당사자가 무효가 되면 권위가 `bHasInteractor` 를 내려 다시 발행하게 한다.
- **확신도**: 중간 — 코드 경로는 확실하다. 영향은 원격 클라이언트에 한정되고, (a) 는 캐릭터의 relevancy 설정에 달려 있다. (b) 는 2인 세션에서 당사자가 이탈하기만 해도 재현될 것으로 보인다.

### 2. 🟡 즉시 완료되는 태그 상태를 `InitialState` 로 지정하면 권위 장치가 발행과 일회성 실행을 계속 멈춘다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:239-253`, `:151`, `:143`, `:404-408`, `:363`
- **범주**: 버그/정확성
- **문제**: 요청 성공 판정은 틱이 끝난 시점에 `LastEnteredTag == 대상` 인지 하나만 본다(권위 `:241`, 클라이언트 `:363`). 그런데 엔진은 `EnterState` 가 완료를 반환하면 같은 틱 안에서 다음 전이까지 이어서 적용한다(엔진 `StateTreeExecutionContext.cpp:1980-2031`). 따라서 복원 진입에서 곧바로 끝나는 태그 상태(스냅하는 이동만 가진 "Opening" 류)가 완료 전이로 다른 태그 상태로 넘어가면 문제가 생긴다. `HandleBeginApplyTransition` 이 대상 진입을 관측했는데도 요청은 거부로 세어진다.

  권위 쪽에서 이런 상태가 `InitialState` 이면 증상이 풀리지 않는다. 요청 세 번 뒤 `FailSynchronization`(`:404-408`)이 걸리지만 `InitialTarget` 은 비워지지 않기 때문이다. 그 결과는 두 가지다.
  - (a) `Synchronize` 가 매 틱 `:249-250` 에서 return 해 `PublishAuthorityState`(`:253`)에 닿지 못한다. `StopLogic` 발행(`:143`)도 막혀서 클라이언트는 이 장치를 한 번도 따라가지 못한다.
  - (b) `TickComponent` 가 매 틱 `bRestoringState` 를 참으로 세운다(`:151`). 그래서 이후 모든 라이브 전이가 복원으로 취급되어, `SendEvent`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners` 가 권위에서 계속 실행되지 않고 이동은 슬라이드 대신 스냅한다.

  에디터 드롭다운(`:475-491`)은 이런 태그 상태를 걸러내지 않고 모두 후보로 보여 준다.
- **제안**: 성공 판정을 "요청 이후 대상 태그 진입을 관측했는가"로 바꾼다. 권위 쪽은 초기 상태 요청이 실패해도 `InitialTarget` 을 비워 발행·라이브 실행으로 돌아가게 한다.
- **확신도**: 높음 — 코드와 엔진 전이 루프로 확인했다. 실제 발생 여부는 전이형 태그 상태를 `InitialState` 로 쓰는 저작이 있는지에 달려 있다.

### 3. 🟡 스포너 정리가 무관한 부착 액터까지 파괴하고, 체크포인트 휴식이 이를 월드 전체로 넓힌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:171-180`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:24-36`
- **범주**: 버그/정확성
- **문제**: 추적하던 `SpawnedActor` 를 파괴한 뒤, 직속 부착 액터를 종류나 생성 주체와 상관없이 전부 `Destroy()` 한다. 이 경로는 `Respawn()`(`:59`)과 `EndPlay()`(`:102`) 양쪽에서 돌고, `TryRespawnAll` 은 Manual 이 아닌 월드의 모든 스포너에 `Respawn()` 을 부른다. 그래서 체크포인트 휴식(`RespawnSpawners` 태스크) 한 번에, 스포너에 붙여 둔 배치 액터(조명·트리거·케이지 장치 등)가 레벨 곳곳에서 함께 사라지고 레벨을 다시 읽기 전까지 돌아오지 않는다.

  주석(`:171`)이 근거로 드는 "스스로 부착하는 적"은 `AWxEnemyCharacter::OnSpawnedBy`(`Source/WxGame/Character/WxEnemyCharacter.cpp:94`)다. 그런데 그 인스턴스는 바로 뒤에 `SpawnTarget()` 이 `SpawnedActor` 에 담는 같은 객체다(`:151`). 이 안전망이 따로 잡아 주는 대상은 사실상 없고 부작용만 남는다.
- **제안**: 부착 목록 순회를 없애고 추적 인스턴스만 정리한다. 보완 탐색을 꼭 남겨야 한다면 `IWxSpawnable` 구현 여부로 대상을 좁힌다. `GetOwner() == this` 검사는 빙의 시 Owner 가 컨트롤러로 바뀌므로(`:154`) 쓸 수 없다. 이 제안은 적의 스포너 부착(확정 설계)을 건드리지 않는다.
- **확신도**: 높음

### 4. 🟡 스포너 발동 태스크만 장치 복원 판정을 쓰지 않아 `InitialState` 복구에서 스폰한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: 이 태스크만 `!Transition.SourceStateID.IsValid()` 하나로 초기 진입을 판정한다. 같은 모듈의 다른 일회성 태스크는 모두 `FWxDeviceExecutionPolicy::IsRestoring`(`Private/Device/WxDeviceExecutionPolicy.cpp:10`)으로 장치의 `bRestoringState` 도 함께 본다. `InitialState` 를 지정한 장치는 시작 직후 `Synchronize` 가 `RequestState(InitialTarget)`(`Private/Device/WxDeviceStateTreeComponent.cpp:249`)으로 전이를 건다. 이 전이는 SourceStateID 가 유효하므로 이 태스크에는 라이브 발동으로 보인다.

  그 결과 레벨 시작 시 권위에서 `Respawn()` 이 돈다. `Respawn()` 은 `SpawnMode` 를 보지 않으므로(`Private/Spawnable/WxSpawner.cpp:51-68`) 트리거를 기다려야 할 Manual 스포너까지 시작부터 적을 스폰한다. 헤더(`Public/Spawnable/WxStateTreeTask_TriggerSpawners.h:27`)의 "초기 진입이면 호출하지 않는다"는 약속과도 어긋난다.
- **제안**: 다른 일회성 태스크처럼 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 로 통일한다. 오너가 장치가 아니면(퀘스트 ST) 기존 판정과 똑같이 동작한다.
- **확신도**: 높음 — 퀘스트 ST 에서는 증상이 없고, 장치 ST 에 이 태스크를 넣을 때 드러난다.

### 5. 🟡 링크 에셋의 태그를 스냅샷에 싣지만 추종은 루트 에셋에서만 한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196-217`, `:451-455`, `:357-361`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState` 는 활성 프레임 전체를 역순으로 훑는다. 그래서 `Frame.StateTree` 가 루트가 아닌 링크 에셋 프레임의 태그 상태도 `LastEnteredTag` 로 채택된다(`:213`). 반면 `HasState` 와 `RequestState` 는 `StateTreeRef.GetStateTree()`(루트 에셋)에만 묻는다(`:451-455`). 루트에 없는 태그가 스냅샷에 실리면 클라이언트는 `:357-361` 에서 `FailSynchronization` 을 찍고 그 장치의 동기화를 멈춘다. `SyncFailure` 는 스냅샷의 serial/tag 가 바뀔 때만 풀리므로, 같은 상태에 머무는 동안에는 복구되지 않는다. 원인은 관측 코드가 헤더(`Public/Device/WxDeviceStateTreeComponent.h:56`)의 "태그는 루트 에셋에서 유일한 상태 식별자" 계약을 지키지 않는 데 있다.
- **제안**: 관측 범위를 루트 에셋 프레임으로 한정한다. 또는 상태 식별·전이 요청에 프레임(에셋) 문맥까지 넣어 링크 에셋 태그도 되짚을 수 있게 한다.
- **확신도**: 중간 — 장치 ST 가 태그 상태를 가진 링크 에셋을 쓸 때만 드러난다.

### 6. 🟡 상태 도중 당사자가 바뀌면 몽타주 대기가 남의 몽타주를 보거나 Failed 로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:42-47`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:43`, `:72`
- **범주**: 버그/정확성
- **문제**: 틱은 재생을 건 캐릭터가 아니라 장치의 `InteractingCharacter` 를 매번 다시 읽는다(`:42-43`). 그런데 그 값은 신호가 전이로 이어지는지와 상관없이 먼저 덮어써진다. `OnInteracted` 는 발행 결과를 보기 전에(`WxDevice.cpp:43`) 대입하고, `NotifyDeviceInteracted` 는 이벤트를 듣는 전이가 없어도 null 까지 그대로(`:72`) 대입한다.

  부모 상태가 켠 상호작용은 자식 상태로 이어진다(`Public/Device/WxDevice.h:69`). 그래서 몽타주 상태 도중 다른 플레이어가 누르면, 권위 쪽 태스크는 그 사람의 AnimInstance 를 폴링하다 곧바로 Succeeded 로 상태를 끊는다. 당사자 없는 장치가 `SendEvent` 를 보내면 값이 null 이 되어 `:44-47` 에서 Failed 가 된다. 진입 시 대상 부재(`:28-31`)와 틱 중 AnimInstance 부재(`:51-54`)는 Succeeded 이고, 헤더(`Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:28-29`)도 "갇히지 않게 완료"를 약속한다. 이 분기만 Failed 라서, 성공 전이만 저작한 장치는 다음 상태로 넘어가지 못한다.

  덧붙여 헤더가 폴링의 근거로 드는 사망·리스폰도 이 검사로는 못 잡는다. `TObjectPtr` 가 GC 전까지 파괴된 포인터를 쥐고 있어 `!Character` 가 걸리지 않기 때문이다.
- **제안**: 진입 시 재생을 건 캐릭터를 인스턴스 데이터에 약참조로 담고 그것만 폴링한다. 무효 판정은 `IsValid` 로 하고 결과는 Succeeded 로 통일한다. `NotifyDeviceInteracted` 가 당사자를 null 로 덮지 않게 하는 것도 함께 검토한다.
- **확신도**: 중간 — 코드 경로는 확실하다. 실제 영향은 몽타주 상태에서 상호작용을 열어 두거나, 당사자 없는 발동 장치를 연결한 저작이 있는지에 달려 있다.

### 7. 🟡 지속 연출용 사운드·Niagara 태스크가 상태를 떠나도 멈추지 않고, 재진입·재시작마다 겹친다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlaySound.h:24-26`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp:36-39`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25-28`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SpawnNiagara.h:38`
- **범주**: 버그/정확성
- **문제**:
  - (a) `PlaySound` 헤더는 `bPlayOnRestore = true` 를 "상태에 묶인 지속 사운드용"이라고 안내한다. 하지만 구현은 핸들을 돌려주지 않는 `UGameplayStatics::PlaySoundAtLocation`(`:38`)이고 `ExitState` 도 없다. 루프 사운드를 넣으면 상태를 떠나도 멈추지 않고, 복원 진입이 반복될 때마다 인스턴스가 겹친다.
  - (b) `SpawnNiagara` 는 "이 노드가 띄운 Niagara 가 재생 중이면 다시 띄우지 않는다"는 판정을 인스턴스 데이터의 `SpawnedComponent`(`:25-28`)에 기댄다. 그런데 엔진은 새로 진입하는 상태의 태스크 인스턴스 데이터를 템플릿에서 새로 만들고, 공통이 아닌 기존 데이터는 버린다(엔진 `StateTreeExecutionContext.cpp:2677-2684`, `:2716-2722`). 그래서 이 판정은 같은 상태의 재선택(Sustained)에서만 통한다. 상태를 떠났다 돌아오거나 트리를 재시작하면 루프 FX 가 하나씩 더 쌓이고, 옛 컴포넌트는 누구도 멈추지 않는다.

  재시작 경로는 클라이언트 재동기화의 `Super::RestartLogic`(`Private/Device/WxDeviceStateTreeComponent.cpp:415-417`)과 2번의 복원 되감기다.
- **제안**: 두 태스크 모두 지속 연출 경로에서는 스폰한 컴포넌트(`SpawnSoundAtLocation`·Niagara 컴포넌트)를 인스턴스 데이터에 두고 `ExitState` 에서 멈춘다. 상태 이후에도 FX 를 남기는 것이 의도라면, 컴포넌트를 장치 쪽에 보관해 중복 판정이 인스턴스 데이터 수명에 기대지 않게 한다. 일회성 전용으로 둘 거라면 헤더 안내를 고친다.
- **확신도**: 중간 — 구현 한계와 엔진 인스턴스 데이터 수명은 확인했다. 실제 영향은 루프 에셋을 쓰는지와 그 상태를 떠나거나 재시작하는 흐름에 달려 있다.

### 8. 🟡 입력 차단 태스크가 HUD 상호작용을 막지 못하고 중첩 차단도 세지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:43-53`, `:58-70`
- **범주**: 설계/구조
- **문제**:
  - (a) `APawn::DisableInput` 은 폰의 입력 스택만 막는다. 상호작용 요청은 HUD 위젯 → `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:85` → 스캐너 `TryInteractSelected`(`Private/Interaction/WxInteractionScannerComponent.cpp:61-70`) → `ServerInteract` 로 흘러 폰 입력을 거치지 않는다. 실질 게이트인 상호작용 어빌리티의 차단 태그(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:30-36`)에도 이 태스크는 아무것도 걸지 않는다. 헤더(`Public/StateTreeTask/WxStateTreeTask_EnablePlayerInput.h:34`)의 "입력 전체"를 믿고 연출 차단에 쓰면, 그 사이 다른 장치와 상호작용할 수 있다.
  - (b) `APawn::EnableInput/DisableInput` 은 불리언 토글이라 횟수를 세지 않는데, `ExitState` 는 무조건 되돌린다. (a) 때문에 연출 중인 장치 A 와 새로 발동한 장치 B 가 같은 폰의 입력을 차례로 끄는 조합이 가능하고, B 가 먼저 끝나면 A 의 연출 도중에 입력이 돌아온다.

  같은 모듈은 상호작용 바인딩에 토큰 스택(`Private/Device/WxDevice.cpp:90-120`)을 두어 같은 문제를 이미 풀어 두었는데, 이 태스크만 그 방식을 따르지 않는다.
- **제안**: 조작 전체를 금지하는 것이 목적이라면, 서버 어빌리티와 로컬 스캐너가 함께 보는 차단 상태(권위에서 부여하는 차단 태그 등)를 태스크 수명 동안 유지하고 중첩 계산도 그 태그에 맡긴다. 폰 입력만 끄는 것이 의도라면 헤더 표현을 좁히고 차단 주체를 토큰으로 센다.
- **확신도**: 중간

### 9. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h`(GameplayStateTreeModule)를 포함하는데, 그 모듈은 Private 의존성이다. 클래스가 export 되지 않아(`:60`, 근거는 `Source/WxEditor/WxDeviceLinkVisualizer.h:12`) 모듈 밖에서 쓸 수도 없다. 따라서 Public 배치가 주는 것은 포함 경로가 보장되지 않는 헤더를 외부에 노출하는 것뿐이다. export 없는 `Public/Device/WxDeviceExecutionPolicy.h` 도 같은 경우다.
- **제안**: 외부에서 쓸 계획이 없다면 두 헤더를 Private 로 옮겨 배치와 의도를 맞춘다. 외부에 열 계획이 있다면 `GameplayStateTreeModule` 을 Public 의존성으로 올리고 export 한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 태스크·시스템 파일(`WxStateTreeTask_PlayAnimation`, `WxStateTreeTask_ApplyGameplayEffectToInteractor`, `WxStateTreeTask_RecordCheckpoint`, `WxStateTreeTask_RespawnSpawners`, `WxSpawnable`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxCheckpointSubsystem`, `WxWorldDeveloperSettings`, `WxWorldModule`)과 대응 헤더 전부.
  - 소비 측 대조: `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`.
  - 엔진 동작 대조(UE 5.8.2): `StateTreeExecutionContext.cpp`(`TickTriggerTransitionsInternal`, 상태 인스턴스 데이터 할당·`ShrinkTo`), `StateTreeExecutionExtension.h`·`StateTreeComponent.h`(확장 교체 시 빠지는 동작 없음 확인), `LevelSequenceActor.cpp`·`LevelSequencePlayer.cpp`(권위 생성 시퀀스 액터가 복제를 끄므로 피어별 로컬 재생이 중복되지 않음 확인).
  - WxCore 태그: 직전 리뷰 이후 `WxGameplayTags` 변경은 `Event.Ability.ActivationStateChanged` 추가와 `Effect.HitStop` 주석 수정뿐이다. WxWorld 가 쓰는 `Event.Interact`·`Ability.Interact` 는 그대로 정의되어 있다.
  - 규칙 점검(57파일 전부): Copyright 첫 줄은 모두 있다. `FORCEINLINE` 과 헤더 인라인 정의는 없다(`GetInstanceDataType()` 과 `TWxStateTreeWaitRegistry` 는 예외 사유 주석이 있다). 람다는 `WxInteractionScannerComponent.cpp:182` 의 정렬 술어 하나다. override 의 `Super::` 호출 누락은 없다. 모듈 밖 Wx 헤더 포함은 `WxCore` 의 `WxInteractable.h`·`WxGameplayTags.h`·`WxLocatorUtils.h` 셋뿐이다.
- **미검토 / 한계**:
  - 이번 검토는 C++ 정적 검토다. 빌드, PIE 재현, StateTree/BP/WBP 에셋 검증은 하지 않았다.
  - 1번은 엔진의 미해소 참조 재통지와 relevancy 설정에, 2·5번은 `InitialState`·링크 에셋 저작에, 6·7·8번은 해당 태스크의 실제 저작에 따라 체감이 달라진다. 콘텐츠 이름 검색으로는 `PlayInteractorMontage`·`EnablePlayerInput`·`PlaySound` 를 쓰는 에셋이 확인되지 않았으므로, 이 항목들은 저작 전에 고칠 잠재 결함으로 보는 편이 맞다.
  - 직전 리뷰의 "처치 상태가 셀 스트리밍으로 리셋되어 `bNeverRevive`·퀘스트 게이트가 뒤집힌다" 항목은 확정 결정(세이브 전면 제거, 세션 내 스트리밍 영속 없음, 리셋은 버그가 아님)과 충돌해 뺐다.
  - 다음 항목은 의도된 설계나 낮은 위험으로 보아 싣지 않았다.
    - `IsAwaited`(`Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:36`)의 `Target` 무검사 역참조: 호출부가 `this` 하나뿐이다.
    - `WxStateTreeTask_RecordCheckpoint.cpp:43` 의 서브시스템 무검사 역참조: 항상 생성되는 GameInstance 서브시스템이다.
    - `AWxDevice::BeginPlay` 가 부모 장치를 암묵적으로 링크하는 것(`Private/Device/WxDevice.cpp:126-132`): 확정 설계다.
    - `PlayAnimation` 의 복원 재생: 헤더에 명시되어 있다.
    - Build.cs 의 쓰이지 않는 `AIModule` 의존성: 사소하다.
  - 장치 액터가 dormancy 없이 복제되는 서버 비용과, 스캐너의 `AllObjects` 오버랩(반경 150cm, 0.1초 주기) 비용은 측정하지 않았다.

---
*문서 기준 커밋 `e0106372a` · 리뷰일 2026-09-16 · 소스 57파일 — `/module-review`로 갱신*
