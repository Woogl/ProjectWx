# WxWorld — 코드 리뷰

> 모듈 경계(`WxCore` 외 Wx 플러그인 무참조)와 CLAUDE.md 코딩 규칙은 57파일 전부에서 깨끗하지만, 장치 상태 동기화 컴포넌트의 "당사자 해소 대기"와 "요청 성공 판정" 두 곳에 장치가 영구히 멈추는 경로가 남아 있다. 직전 리뷰 이후 C++ 변경은 주석뿐이라 기존 발견을 현재 코드로 전부 재검증했고(헤더 정정으로 해소된 `PlayAnimation` 정책 불일치·`SpawnNiagara` 루프 FX 항목은 제외), 이번에는 `InitialState` 요청 판정·당사자 교체·복원 재진입 경로를 엔진 StateTree 전이 루프와 대조해 새로 팠다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 8 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 해소되지 않는 당사자 참조가 클라이언트의 장치 상태 추종을 영구 차단한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:340-344`, `:318-322`, `:282-284`, `:264-268`
- **범주**: 설계/구조
- **문제**: `FollowAuthorityState` 는 `StateSnapshot.bHasInteractor` 가 참이고 `Interactor` 가 무효면 무조건 return 한다(`:340-344`). 주석은 매핑 완료 후의 RepNotify 재적용에 기대지만, 그 재통지는 참조가 언젠가 해소될 때만 온다. 해소되지 않는 경로가 둘 있다. (a) 당사자 캐릭터가 그 클라이언트의 relevancy 밖이면 NetGUID 가 미해소로 남는다 — 문을 연 플레이어가 멀어진 뒤 다른 플레이어가 그 문에 다가가 스냅샷을 처음 받는 순서가 그대로 이 경우다. (b) 당사자가 파괴(사망·접속 종료)되면 클라이언트의 `Interactor` 는 무효가 되는데 `bHasInteractor` 는 참으로 남는다 — 발행은 새 진입이나 RunStatus 변화에서만 다시 일어나고(`:264-268`), 그때도 판정이 `IsValid` 가 아닌 `Interactor != nullptr`(`:284`)라 GC 전의 파괴된 포인터를 "당사자 있음"으로 싣는다. 어느 쪽이든 클라이언트는 `RequestState` 에 닿지 못해, 문이 서버에선 열려 있고 그 클라이언트에선 닫힌 채 다음 발행까지 어긋나며 로컬 트리의 상호작용 바인딩도 옛 상태로 남아 스캐너 프롬프트까지 틀린다. `SyncAttempts`·`SyncFailure` 를 거치지 않는 경로라 에러 로그도 없고, 진단은 `DescribeSynchronization` 의 `waitingInteractor` 플래그(`:463`)뿐이다.
- **제안**: 당사자 해소를 상태 수렴의 전제조건에서 떼어 상태는 태그만으로 적용하고 당사자는 해소되는 대로 뒤늦게 채운다(당사자를 읽는 몽타주·GE·입력·체크포인트 태스크는 당사자 없는 진입 경로를 이미 갖고 있다). 최소 조치로는 `:284` 를 `IsValid` 판정으로 바꾸고, 대기에 시한을 두어 만료 시 당사자 없이 진행한다.
- **확신도**: 중간 — 코드 경로는 확실하나 (a) 는 당사자 캐릭터의 relevancy 설정에 달려 있다. (b) 는 2인 세션에서 당사자의 사망·이탈만으로 재현 가능해 보인다.

### 2. 🟡 즉시 완료되는 상태를 `InitialState` 로 지정하면 권위 장치가 발행과 일회성 실행을 영구히 멈춘다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:239-253`, `:151`, `:404-408`, `:363`
- **범주**: 버그/정확성
- **문제**: 요청 성공 판정은 틱이 끝난 시점의 `LastEnteredTag == 대상`(권위 `:241`, 클라이언트 `:363`) 하나뿐이다. 엔진은 `EnterState` 가 완료를 반환하면 같은 틱 안에서 다음 전이까지 이어서 적용하므로(`StateTreeExecutionContext.cpp` 의 `TickTriggerTransitionsInternal` 반복 루프), 복원 진입에서 곧바로 끝나는 상태 — `ComponentMove`·`SplineMove` 스냅이나 복원 시 즉시 Succeeded 인 몽타주·시퀀스 태스크만 가진 "Opening" 류 — 가 완료 전이로 다른 태그 상태로 넘어가면, 대상 태그를 실제로 거쳐 갔는데도(`HandleBeginApplyTransition` 이 그 진입을 관측한다) 요청이 거부된 것으로 세어진다. 대상 아래의 태그 자식이 자동 선택되는 부모 상태도 같다. 권위 쪽에서 이런 상태가 `InitialState` 면 결과가 영구적이다 — 요청 세 번 뒤 `FailSynchronization`(`:404-408`)이 걸리지만 `InitialTarget` 은 비워지지 않아, (a) `Synchronize` 가 매 틱 `:249-250` 에서 return 해 `PublishAuthorityState`(`:253`)에 닿지 못하고 `StopLogic` 발행(`:143`)도 막히므로 클라이언트는 이 장치를 한 번도 따라가지 못하고, (b) `TickComponent` 가 `InitialTarget.IsValid()` 로 `bRestoringState` 를 매 틱 참으로 세워(`:151`) 이후 모든 라이브 전이가 복원으로 취급된다 — `SendEvent`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners` 가 권위에서 영구히 침묵하고, 리슨 호스트·싱글플레이에선 기본 설정 사운드·시퀀스·몽타주도 재생되지 않으며 이동은 슬라이드 대신 스냅한다. 에디터 드롭다운(`:475-491`)은 태그 상태를 가리지 않고 모두 후보로 보여 준다. 클라이언트도 같은 판정을 써서, 레이트조인의 첫 스냅샷이 이런 상태를 가리키면 되감기 세 번 끝에 Error 로그와 함께 다음 스냅샷까지 동기화가 멈춘다.
- **제안**: 성공을 "요청 이후 대상 태그 진입을 관측했는가"로 판정한다. 권위 쪽은 초기 상태 요청이 실패해도 `InitialTarget` 을 비워 발행·라이브 실행으로 복귀시킨다. 클라이언트는 적용 뒤 완료 전이로 앞서 나간 경우를 되감기 대상으로 볼지 함께 정해야 한다.
- **확신도**: 높음 — 권위 경로는 코드와 엔진 전이 루프로 확인했다. 발생 여부는 전이형 태그 상태를 `InitialState` 로 쓰는 저작이 있는지에 달려 있다.

### 3. 🟡 스포너 정리가 무관한 부착 액터까지 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:171-180`
- **범주**: 버그/정확성
- **문제**: 추적하던 `SpawnedActor` 를 파괴한 뒤 직속 부착 액터를 종류·생성 주체를 가리지 않고 전부 `Destroy()` 한다. 이 경로는 `Respawn()`(`:59`)과 `EndPlay()`(`:102`) 양쪽에서 돈다. 스포너에 조명·트리거·케이지 장치 같은 배치 액터를 붙여 둔 레벨에서는 리스폰 한 번에 그것들이 함께 사라지고, 배치 액터라 레벨을 다시 읽기 전까지 돌아오지 않는다. 주석(`:171`)이 근거로 드는 "스스로 부착하는 적"은 `AWxEnemyCharacter::OnSpawnedBy`(`Source/WxGame/Character/WxEnemyCharacter.cpp:94`)인데, 그 인스턴스는 직후 `SpawnTarget()` 이 `SpawnedActor` 에 담는 바로 그 객체라(`:151`) 이 안전망이 따로 잡아 주는 대상은 사실상 없고 부작용만 남는다.
- **제안**: 부착 목록 순회를 제거하고 추적 인스턴스만 정리한다. 보완 탐색을 유지하려면 "이 스포너가 만든 인스턴스"라는 명시적 표식(`GetOwner() == this` 검사나 전용 태그)을 요구한다.
- **확신도**: 높음

### 4. 🟡 스포너 발동이 장치의 초기 상태 복구에서도 실행된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: 이 태스크만 `!Transition.SourceStateID.IsValid()` 단독으로 초기 진입을 판정한다. 같은 모듈의 다른 일회성 태스크는 전부 `FWxDeviceExecutionPolicy::IsRestoring`(`Private/Device/WxDeviceExecutionPolicy.cpp:10`)을 써서 장치의 `bRestoringState` 도 함께 본다. `InitialState` 를 지정한 장치는 시작 직후 `Synchronize` 가 `RequestState(InitialTarget)`(`Private/Device/WxDeviceStateTreeComponent.cpp:249`)으로 전이를 거는데, 그 전이는 SourceStateID 가 유효하므로 이 태스크에는 라이브 발동으로 보인다. 그 결과 레벨 시작 시 권위에서 `Respawn()` 이 돌고, `Respawn()` 은 `SpawnMode` 를 보지 않으므로(`Private/Spawnable/WxSpawner.cpp:51-68`) 트리거를 기다려야 할 Manual 스포너까지 시작부터 적을 스폰한다. 헤더(`Public/Spawnable/WxStateTreeTask_TriggerSpawners.h:27`)의 "초기 진입이면 호출하지 않는다" 약속과도 어긋난다.
- **제안**: 다른 일회성 태스크와 같이 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 으로 통일한다(오너가 장치가 아니면 기존 판정과 동일하게 동작한다).
- **확신도**: 높음

### 5. 🟡 처치 상태가 셀 스트리밍으로 리셋되어 영구 처치·퀘스트 게이트가 뒤집힌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h:61-66`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:90-98`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_WaitSpawnersKilled.h:34`
- **범주**: 설계/구조
- **문제**: `bIsKilled` 는 순수 런타임 멤버이고 어디에도 보관되지 않는다(`MarkKilled` 의 유일한 호출자는 `Source/WxGame/Character/WxEnemyCharacter.cpp:172`). 헤더 `:65` 가 "셀이 스트림 아웃되면 함께 사라진다"고 이제 명시하지만, 그 결과를 받는 쪽의 약속은 그대로다. 스트림 인된 스포너는 `bIsKilled = false` 로 새로 만들어져 `BeginPlay` 의 Auto 경로(`WxSpawner.cpp:94-97`)가 곧바로 재스폰하므로, (a) `bNeverRevive`(`:61-63`)로 표시한 보스가 플레이어가 한 바퀴 돌아오면 되살아나 그 플래그가 셀 경계 안에서만 유효하고, (b) 여러 셀에 걸친 `FWxStateTreeTask_WaitSpawnersKilled` 퀘스트 게이트가 역행한다 — A 셀 적을 처치하고 B 셀로 가 A 가 언로드된 사이 B 를 처치해도 A 가 미해석이라 대기하고, A 로 돌아오면 미처치로 되살아나 있다. 그 태스크 헤더 `:34` 는 저주기 폴링의 근거로 "스트리밍 인처럼 이벤트 없는 상태 변화도 놓치지 않는다"고 적어 두었는데, 이는 스트리밍이 처치 상태를 보존한다는 전제에서만 성립한다.
- **제안**: 처치 상태를 셀 밖(월드 서브시스템의 스포너 GUID→처치 여부 맵 등)으로 올려 액터 수명과 분리한다. 현 동작을 유지한다면 `bNeverRevive` 주석과 `WaitSpawnersKilled` 헤더 서술을 정정해 오용을 막는다.
- **확신도**: 중간 — 보관 경로가 없는 것은 확실하고, 체감은 실제 셀 분할과 퀘스트·보스 배치에 달려 있다.

### 6. 🟡 링크 에셋의 태그를 스냅샷에 싣지만 루트 에셋에서만 추종한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196-217`, `:451-455`, `:357-361`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState` 는 활성 프레임 전체를 역순으로 훑어, `Frame.StateTree` 가 루트가 아닌 링크 에셋 프레임의 태그 상태까지 `LastEnteredTag` 로 채택한다(`:213`). 반면 `HasState` 와 `RequestState` 는 `StateTreeRef.GetStateTree()`(루트 에셋)에만 질의한다(`:451-455`). 루트에 없는 태그가 스냅샷에 실리면 클라이언트는 `:357-361` 에서 `FailSynchronization` 을 찍고 그 장치의 동기화를 멈추며, `SyncFailure` 는 스냅샷의 serial/tag 가 바뀔 때만 풀리므로 같은 상태에 머무는 동안 복구되지 않는다. 헤더(`Public/Device/WxDeviceStateTreeComponent.h:56`)가 선언한 "태그는 루트 에셋에서 유일한 상태 식별자" 계약을 관측 코드가 지키지 않는 것이 원인이다.
- **제안**: 관측 범위를 루트 에셋 프레임으로 한정하거나, 상태 식별·전이 요청에 프레임(에셋) 문맥까지 포함해 링크 에셋 태그도 되짚을 수 있게 한다.
- **확신도**: 중간

### 7. 🟡 상태 도중 당사자가 바뀌면 몽타주 대기가 남의 몽타주를 보거나 실패로 끝난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:42-47`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:43`, `:72`
- **범주**: 버그/정확성
- **문제**: 틱은 재생을 건 캐릭터가 아니라 장치의 `InteractingCharacter` 를 매번 다시 읽는다(`:42-43`). 그런데 그 값은 신호가 전이로 이어지는지와 무관하게 먼저 덮인다 — `OnInteracted` 는 발행 결과를 보기 전에(`WxDevice.cpp:43`), `NotifyDeviceInteracted` 는 그 이벤트를 듣는 전이가 없어도 null 까지 그대로(`:72`) 대입한다. 부모 상태가 켠 상호작용은 자식 상태로 이어지므로(`Public/Device/WxDevice.h:69`) 몽타주 상태 도중 다른 플레이어가 누르면 권위의 태스크는 그 사람의 AnimInstance 를 폴링해 즉시 Succeeded 로 상태를 끊고, 당사자 없는 장치가 `SendEvent` 를 보내면 null 이 되어 `:44-47` 에서 Failed 가 된다. 진입 시 대상 부재(`:28-31`)와 틱 중 AnimInstance 부재(`:51-54`)는 Succeeded 이고 헤더(`Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:28-29`)도 "갇히지 않게 완료"를 내세우는데 이 분기만 Failed 라, 성공 전이만 저작한 장치는 다음 상태로 못 간다. 교체된 당사자는 다음 진입 전까지 발행되지 않아(`Private/Device/WxDeviceStateTreeComponent.cpp:264-268`) 클라이언트는 옛 당사자로 계속 폴링한다. 한편 헤더가 폴링의 근거로 드는 사망·리스폰은 `TObjectPtr` 가 GC 전까지 파괴된 포인터를 쥐고 있어 `!Character` 검사로는 잡히지 않는다.
- **제안**: 진입 시 재생을 건 캐릭터를 인스턴스 데이터에 약참조로 담아 그것만 폴링하고, 무효 판정은 `IsValid` 로 하되 Succeeded 로 통일한다. `NotifyDeviceInteracted` 가 당사자를 null 로 덮지 않게 하는 것도 함께 검토한다.
- **확신도**: 중간 — 코드 경로는 확실하고, 체감은 몽타주 상태에서 상호작용을 열어 두거나 당사자 없는 발동 장치를 연결한 저작이 있는지에 달려 있다.

### 8. 🟡 `bPlayOnRestore` 사운드는 상태에 묶인 지속 사운드를 표방하지만 멈추지도 중복을 막지도 못한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlaySound.h:24-26`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp:36-39`
- **범주**: 버그/정확성
- **문제**: 헤더는 `bPlayOnRestore = true` 를 "상태에 묶인 지속 사운드용"이라 안내하지만, 구현은 핸들을 돌려주지 않는 `UGameplayStatics::PlaySoundAtLocation`(`:38`)이고 `ExitState` 도 없다. 루프 사운드를 넣으면 상태를 떠나도 멈추지 않고, 복원 진입이 반복될 때마다 인스턴스가 겹친다 — 클라이언트 재동기화의 `RestartLogic`(`Private/Device/WxDeviceStateTreeComponent.cpp:415-417`), 2번의 복원 되감기, 재선택(`bShouldStateChangeOnReselect` 기본값 유지)마다 새로 튼다. 같은 모듈의 `SpawnNiagara` 는 적어도 재생 중이면 다시 띄우지 않는다(`Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:25-28`).
- **제안**: `bPlayOnRestore` 경로는 `SpawnSoundAtLocation` 으로 컴포넌트를 받아 인스턴스 데이터에 두고, 재생 중이면 건너뛰며 `ExitState` 에서 멈춘다. 일회성 전용으로 둘 거라면 헤더 안내를 고친다.
- **확신도**: 중간 — 구현 한계는 확실하고, 실제 루프 사운드 저작 여부는 에셋에 달려 있다.

### 9. 🟡 입력 차단 태스크가 HUD 상호작용을 막지 못하고 중첩 차단도 세지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:43-53`, `:58-70`
- **범주**: 설계/구조
- **문제**: (a) `APawn::DisableInput` 은 폰의 `bInputEnabled` 만 내린다. 상호작용 요청은 HUD 위젯 → `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:85` → 스캐너 `TryInteractSelected`(`Private/Interaction/WxInteractionScannerComponent.cpp:61-70`) → `ServerInteract` 로 흘러 폰 입력을 거치지 않고, 실질 게이트인 상호작용 어빌리티의 `CanActivateAbility`(`:302-320`)에도 이 태스크는 아무 태그를 걸지 않는다. 헤더(`Public/StateTreeTask/WxStateTreeTask_EnablePlayerInput.h:34`)의 "입력 전체"를 믿고 컷신 차단에 쓰면 그 사이 다른 장치와 상호작용할 수 있다. (b) `APawn::EnableInput/DisableInput` 은 불리언 토글이라 카운트가 없는데 `ExitState` 는 무조건 되돌린다. (a) 때문에 연출 중인 장치 A 와 새로 발동한 장치 B 가 같은 폰을 차례로 끄는 조합이 성립하고, B 가 먼저 끝나면 A 의 연출 중간에 입력이 돌아온다. 같은 모듈이 상호작용 바인딩에는 토큰 스택(`Private/Device/WxDevice.cpp:90-120`)을 두어 같은 문제를 이미 풀어 둔 것과 정책이 갈린다.
- **제안**: 전체 조작 금지가 목적이라면 서버 어빌리티와 로컬 스캐너가 함께 보는 차단 상태(권위에서 부여하는 차단 태그 등)를 태스크 수명 동안 유지해 카운트까지 그 태그에 맡긴다. 폰 입력만 끄는 것이 의도라면 헤더 표현을 좁히고 차단 주체를 토큰으로 센다.
- **확신도**: 중간

### 10. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h`(GameplayStateTreeModule)를 포함하는데 그 모듈은 Private 의존성이다. 클래스는 export 되지 않아(`:60`, 근거는 `Source/WxEditor/WxDeviceLinkVisualizer.h:12`) 모듈 밖에서 쓸 수도 없으므로, Public 배치가 주는 것은 포함 경로가 보장되지 않는 헤더를 외부에 노출하는 것뿐이다. 같은 이유로 `Public/Device/WxDeviceExecutionPolicy.h` 도 export 없이 Public 에 있다.
- **제안**: 외부 소비 계획이 없다면 두 헤더를 Private 로 옮겨 배치와 의도를 맞춘다. 열 계획이 있다면 `GameplayStateTreeModule` 을 Public 의존성으로 승격하고 export 한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 태스크·시스템 파일(`WxStateTreeTask_PlayAnimation`, `WxStateTreeTask_ApplyGameplayEffectToInteractor`, `WxStateTreeTask_RecordCheckpoint`, `WxStateTreeTask_RespawnSpawners`, `WxSpawnable`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxCheckpointSubsystem`, `WxWorldDeveloperSettings`, `WxWorldModule`)과 대응 헤더 전부. 소비 측 대조로 `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp` 를, 엔진 동작 대조로 UE 5.8 의 `StateTreeExecutionContext.cpp`(`TickTriggerTransitionsInternal`), `StateTreeComponent.cpp`(`StartTree`·`TickComponent`·확장 교체), `Pawn.cpp`(`EnableInput`/`DisableInput`), `DataChannel.cpp`(relevancy 채널 정리)를 읽었다. 규칙 점검은 57파일 전부를 대상으로 했다 — Copyright 첫 줄 전부 존재, `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 16건과 `TWxStateTreeWaitRegistry` 는 예외 사유 주석 있음), `BlueprintCallable` 은 BP Function Library 의 `UWxSpawnerLibrary::TryRespawnAll` 한 건, 람다는 `WxInteractionScannerComponent.cpp:182` 정렬 술어 하나, 콜백은 `HandleScanTimer`·`HandleBeginApplyTransition` 로 `Handle` prefix, override 의 `Super::` 호출 누락 없음, 모듈 밖 Wx 헤더 포함은 `WxCore` 의 `WxInteractable.h`·`WxGameplayTags.h`·`WxLocatorUtils.h` 셋뿐이다.
- **미검토 / 한계**: C++ 정적 검토이며 빌드·PIE 재현·StateTree/BP/WBP 에셋 검증은 하지 않았다. 1번은 엔진의 미해소 참조 재통지와 relevancy 설정에, 2번은 `InitialState`·전이형 태그 상태 저작에, 5번은 WP 셀 분할과 퀘스트 배치에, 6·7·8번은 링크 에셋·몽타주 상태 상호작용·루프 사운드 저작 여부에 체감이 달려 있어 2인 이상 PIE 와 에셋 확인이 필요하다. 직전 리뷰의 `PlayAnimation` 항목은 헤더가 복원·재진입 재생을 명시하도록 정정되어, `SpawnNiagara` 루프 FX 항목은 헤더가 루프 유지를 명시해 의도로 보고 제외했다. `IsAwaited`(`Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:36`)의 `Target` 무검사 역참조는 호출부가 `this` 하나뿐이라, `WxStateTreeTask_RecordCheckpoint.cpp:43` 의 서브시스템 무검사 역참조는 항상 생성되는 GameInstance 서브시스템이라, `AWxDevice::BeginPlay` 의 부모 장치 암묵 링크(`Private/Device/WxDevice.cpp:126-132`)는 의도된 편의로 보아 제외했다. `AWxNpc::CanInteract` 가 권위 전용 대기 등록부에 기대는 전제는 호출부 주석(`Source/WxGame/Character/WxNpc.cpp:42`)에 명시되어 있고 모듈 밖이라 싣지 않았다. 장치 액터가 dormancy 없이 복제되는 서버 비용과 스캐너의 `AllObjects` 오버랩(반경 150cm, 0.1초 주기) 비용은 재 보지 않았다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 57파일 — `/module-review`로 갱신*
