# WxWorld — 코드 리뷰

> 직전 리뷰 이후 C++ 변경이 없어(바뀐 것은 `README.md` 뿐) 재검증 패스로 진행했고, 이번에는 상호작용 컴포넌트·태스크의 수명주기와 실패 경로, 장치 스냅샷의 리플리케이션 권위, 셀 스트리밍 시 스폰 정리 경로를 새로 파는 데 시간을 썼다. 규칙 위반은 여전히 한 건도 없고 모듈 경계(`WxCore` 외 Wx 플러그인 무참조)도 깨끗하지만, 장치 스냅샷의 "당사자 해소 대기"에 시한이 없어 클라이언트가 영구히 상태를 못 따라가는 경로를 새로 찾았다. 이번 리뷰는 소스 57파일 전부를 열었고 장치 동기화·스포너 수명·스캐너·태스크군 네 축의 cpp 를 라인 단위로 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 8 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 해소되지 않는 당사자 참조가 클라이언트의 장치 상태 추종을 영구 차단한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:340-344`, `:318-322`, `:283-284`
- **범주**: 설계/구조
- **문제**: `FollowAuthorityState` 는 `StateSnapshot.bHasInteractor` 가 참이고 `Interactor` 가 무효면 **무조건 return** 한다(`:340-344`). 주석은 "매핑 완료 후 RepNotify 가 다시 적용한다"는 엔진의 미해소 참조 재통지에 기대고 있는데, 그 재통지는 참조가 **언젠가 해소될 때만** 온다. 해소되지 않는 경로가 실재한다 — (a) 당사자 캐릭터가 그 클라이언트의 net relevancy 밖이면 NetGUID 가 영구 미해소다. 문을 연 플레이어가 멀어진 뒤 다른 플레이어가 그 문에 접근하는(= 장치가 relevant 해지는) 순서가 그대로 이 경우다. (b) 당사자가 서버에서 파괴(사망·리스폰·접속 종료)되면 권위의 `Interactor` 는 GC 로 null 이 되지만 `bHasInteractor` 는 참으로 남는다 — `PublishAuthorityState` 가 그 두 값을 **새 진입이나 RunStatus 변화에서만** 갱신하기 때문이다(`:264-284`). 어느 쪽이든 클라이언트는 `bHasAppliedSnapshot` 이 false 인 채 `RequestState` 에 닿지 못해 그 장치의 상태를 한 번도 적용하지 않는다. 문이 서버에선 열려 있고 그 클라이언트에선 닫힌 채로, **다음 상호작용이 일어날 때까지 영구히** 어긋난다. `SyncAttempts`·`SyncFailure` 는 이 경로를 세지 않으므로 에러 로그조차 없다(진단은 `DescribeSynchronization` 의 `waitingInteractor` 플래그뿐, `:463`).
- **제안**: 당사자 해소를 상태 수렴의 전제조건에서 떼어낸다 — 상태는 태그만으로 적용하고 당사자는 해소되는 대로 뒤늦게 채운다(당사자를 실제로 읽는 태스크는 몽타주·GE·입력뿐이고 모두 없으면 조기 완료하도록 이미 만들어져 있다). 최소 조치로는 대기에 시한을 두어 만료 시 당사자 없이 진행하고, 권위에서 당사자가 사라졌을 때 `bHasInteractor` 를 내려 재발행한다.
- **확신도**: 중간 — 코드 경로는 확실하나, 프로젝트가 당사자 캐릭터를 항상 relevant 로 유지한다면 (a) 는 발생하지 않는다. (b) 는 싱글 클라이언트 세션에서도 재현 가능해 보인다.

### 2. 🟡 스포너 정리가 무관한 부착 액터까지 파괴한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:171-180`
- **범주**: 버그/정확성
- **문제**: 추적하던 `SpawnedActor` 를 파괴한 뒤, 직속 부착 액터를 종류·생성 주체를 가리지 않고 전부 `Destroy()` 한다. 이 경로는 `Respawn()`(`:59`)과 `EndPlay()`(`:102`) 양쪽에서 돈다. 스포너에 조명·연출·트리거 등 다른 액터를 붙여 둔 배치에서는 리스폰 한 번에 그것들이 함께 사라진다. 게다가 주석이 말하는 "약참조를 놓친 경우"는 실제로 발생하지 않는다 — `AWxEnemyCharacter::OnSpawnedBy`(`Source/WxGame/Character/WxEnemyCharacter.cpp:93`)가 스스로 부착하지만 그 직후 `SpawnTarget()` 이 같은 인스턴스를 `SpawnedActor` 에 담으므로(`:151`), 이 안전망이 잡아 주는 대상은 사실상 없고 부작용만 남는다.
- **제안**: 부착 목록 순회를 제거하고 추적 인스턴스만 정리한다. 보완 탐색을 유지하려면 "이 스포너가 만든 인스턴스"라는 명시적 표식(예: `GetOwner() == this` 검사나 전용 태그)을 요구한다.
- **확신도**: 높음

### 3. 🟡 스포너 발동이 장치의 초기 상태 복구에서도 실행된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp:24`
- **범주**: 버그/정확성
- **문제**: 이 태스크만 `!Transition.SourceStateID.IsValid()` 단독으로 초기 진입을 판정한다. 같은 모듈의 다른 일회성 태스크는 전부 `FWxDeviceExecutionPolicy::IsRestoring`(`Private/Device/WxDeviceExecutionPolicy.cpp:10`)을 쓰며, 그쪽은 장치의 `bRestoringState` 도 함께 본다. `InitialState` 를 지정한 장치는 시작 직후 `Synchronize` 가 `RequestState(InitialTarget)`(`Private/Device/WxDeviceStateTreeComponent.cpp:249`)으로 전이를 거는데, 그 전이는 SourceStateID 가 유효하므로 이 태스크에는 "라이브 발동"으로 보인다. 결과적으로 레벨 시작 시 권위 측에서 `Respawn()` 이 돌아 기존 스폰 대상을 파괴하고 다시 만든다.
- **제안**: 다른 일회성 태스크와 같이 `FWxDeviceExecutionPolicy::IsRestoring(Context, Transition)` 으로 통일한다.
- **확신도**: 높음

### 4. 🟡 처치 상태가 셀 스트리밍으로 리셋되어 영구 처치·퀘스트 게이트가 뒤집힌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h:65-66`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:90-98`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_WaitSpawnersKilled.h:36`
- **범주**: 설계/구조
- **문제**: `bIsKilled` 는 순수 런타임 멤버이고 어디에도 보관되지 않는다(`MarkKilled` 의 유일한 호출자는 `Source/WxGame/Character/WxEnemyCharacter.cpp:172`). 스포너가 놓인 WP 셀이 스트림 아웃되면 액터가 파괴되고, 다시 스트림 인될 때는 패키지 기본값(`bIsKilled = false`)으로 새로 만들어져 `BeginPlay` 의 Auto 경로가 곧바로 재스폰한다. 결과는 두 가지다 — (a) `bNeverRevive` 로 표시한 보스가 플레이어가 한 바퀴 돌아오면 되살아나므로 그 플래그가 셀 경계 안에서만 유효하다. (b) `FWxStateTreeTask_WaitSpawnersKilled` 로 만든 퀘스트 게이트가 **역행**한다 — 전원 처치로 통과 직전이었던 대기가 셀 왕복 후 다시 미처치로 돌아간다. 특히 그 태스크 헤더 `:36` 은 저주기 폴링의 근거로 "스트리밍 인처럼 별도 처치 이벤트가 없는 상태 변화도 놓치지 않는다"고 적어 두었는데, 그 문장은 스트리밍이 처치 상태를 보존한다는 전제에서만 성립한다.
- **제안**: 처치 상태를 셀 밖(예: 월드 서브시스템에 스포너 GUID→처치 여부 맵, 또는 `AWxSpawner` 를 WP 퍼시스턴스 대상으로)으로 올려 액터 수명과 분리한다. 현 동작을 유지할 경우 최소한 `bNeverRevive` 의 유효 범위와 `WaitSpawnersKilled` 헤더의 서술을 정정해 오용을 막는다.
- **확신도**: 중간 — 코드에 보관 경로가 없는 것은 확실하고, 체감 여부는 실제 셀 분할 크기와 퀘스트 배치에 달려 있다.

### 5. 🟡 링크 에셋의 태그를 스냅샷에 싣지만 루트 에셋에서만 추종한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:196-222`, `:451-455`
- **범주**: 설계/구조
- **문제**: `ObserveActiveState` 는 활성 프레임 전체를 역순으로 훑어 링크 에셋 안의 태그 상태까지 `LastEnteredTag` 로 채택한다. 반면 `HasState` 와 `RequestState` 는 `StateTreeRef.GetStateTree()`(루트 에셋)에만 질의한다. 루트에 없는 태그가 스냅샷에 실리면 클라이언트의 `FollowAuthorityState` 가 `:357` 에서 `FailSynchronization` 을 찍고(Error 로그) 그 장치의 동기화가 영구 중단된다 — `SyncFailure` 는 스냅샷의 serial/tag 가 바뀔 때만 풀리므로 같은 스냅샷으로는 복구되지 않는다. 헤더가 선언한 "태그는 루트 에셋에서 유일한 상태 식별자" 계약을 관측 코드가 지키지 않는 것이 원인이다.
- **제안**: 관측 범위를 루트 에셋 프레임으로 한정하거나, 상태 식별·전이 요청에 프레임(에셋) 문맥까지 포함해 링크 에셋 태그도 되짚을 수 있게 한다.
- **확신도**: 중간

### 6. 🟡 몽타주 도중 대상 소실이 정상 종료 대신 실패로 전파된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:44-46`
- **범주**: 버그/정확성
- **문제**: 진입 시 대상 부재(`:28-31`)와 틱 중 AnimInstance 부재(`:51-54`)는 Succeeded 인데, 틱 중 Character 부재만 Failed 다. 헤더 `:29` 는 "폴링은 대상이 사라진 것까지 종료로 본다"고 이 설계의 근거를 명시하고 있어 코드가 자기 계약을 어긴다. 성공 전이만 저작한 장치는 당사자 사망·리스폰으로 다음 상태에 못 가고 실패가 상위로 전파되며, 장치 컴포넌트는 완료 상태도 복제하므로 클라이언트까지 멈춘다.
- **제안**: 대상 소실을 Succeeded 로 통일한다.
- **확신도**: 높음

### 7. 🟡 애니메이션 재생의 복구·재선택 정책이 이동 태스크와 다르다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp:26`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_PlayAnimation.h:33`
- **범주**: 버그/정확성
- **문제**: 헤더는 "'Component Move' 와 동일한 방침"이라 적었지만 두 태스크가 실제로 다르다. `ComponentMove` 는 `IsRestoringDevice` 일 때 목표 포즈로 스냅하고(`WxStateTreeTask_ComponentMove.cpp:37`) 재선택 시 재진입을 억제한다(`:14`, `bShouldStateChangeOnReselect = false`). `PlayAnimation` 은 복구 판정 자체가 없고 생성자도 없어(재선택 억제 설정 없음) 레이트조인·초기 상태 복원에서 처음부터 재생하고, 같은 상태가 재선택될 때마다 애니메이션이 되감긴다. 두 태스크를 한 상태에 조합한 문·엘리베이터는 몸통은 열린 채로 스냅되는데 애니메이션은 여는 동작을 다시 트는 그림이 된다.
- **제안**: 복구 시 끝 포즈 적용 여부와 재선택 시 재생 여부를 명시적으로 정해 `ComponentMove` 와 맞추거나, 현 동작을 유지할 경우 헤더의 "동일한 방침" 서술을 정정한다.
- **확신도**: 중간

### 8. 🟡 폰 입력 차단이 HUD 상호작용 경로를 덮지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:49`
- **범주**: 설계/구조
- **문제**: `DisableInput` 은 폰에 걸린 입력만 끈다. 상호작용 요청은 HUD 위젯 → `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:85` → 스캐너 `TryInteractSelected`(`Private/Interaction/WxInteractionScannerComponent.cpp:61-70`) → `ServerInteract` 로 흘러 폰 입력을 한 번도 거치지 않는다. 컷신처럼 "조작 전체 금지"를 의도해 이 태스크를 쓰면 상호작용은 계속 가능하다. 실질적인 차단은 스캐너가 매 스캔에서 묻는 상호작용 어빌리티의 `CanActivateAbility`(`:302-320`)뿐인데, 이 태스크는 그 판정에 영향을 주는 어떤 태그도 붙이지 않는다.
- **제안**: 전체 조작 금지가 목적이라면 서버 어빌리티와 로컬 스캐너가 함께 보는 차단 상태(태그)를 태스크 수명 동안 유지하도록 넓힌다. 폰 입력만 끄는 것이 의도라면 헤더의 "입력 전체" 표현을 좁혀 오용을 막는다.
- **확신도**: 중간

### 9. 🟡 입력 차단이 중첩을 세지 않아 먼저 끝난 쪽이 남의 차단을 되돌린다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp:58-70`
- **범주**: 설계/구조
- **문제**: `ExitState` 는 자기가 끈 폰에 무조건 `EnableInput` 을 건다. `AActor::EnableInput/DisableInput` 은 참조 카운트가 없으므로, 같은 폰을 두 장치가 동시에 차단한 뒤 한쪽이 먼저 상태를 떠나면 나머지 차단이 함께 풀린다. 8번과 맞물려 도달 경로가 구체적이다 — 입력이 꺼진 동안에도 HUD 경로로 다른 장치와 상호작용할 수 있으므로, 연출 중인 장치 A 와 새로 발동한 장치 B 가 같은 폰의 입력을 차례로 끄는 조합이 성립하고, B 가 먼저 끝나면 A 의 연출 중간에 입력이 돌아온다. 같은 모듈이 상호작용 바인딩에는 토큰 스택(`AWxDevice::PushInteractionBinding`/`PopInteractionBinding`, `Private/Device/WxDevice.cpp:92-122`)을 두어 이 문제를 이미 해결해 두었다는 점에서 정책이 갈린다.
- **제안**: 상호작용 바인딩과 같은 토큰/카운트 방식으로 차단 주체를 세고, 마지막 차단이 풀릴 때만 입력을 되돌린다. 8번을 차단 태그 방식으로 고칠 경우 카운트를 그 태그에 얹으면 한 번에 해결된다.
- **확신도**: 중간

### 10. 🟢 루프 Niagara 를 상태 이탈 시 끌 수단이 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp:36-46`
- **범주**: 설계/구조
- **문제**: 태스크가 컴포넌트를 생성하지만 `ExitState` 정리 경로가 없다(헤더에도 선언 없음). 자동 파괴는 시스템 완료를 전제하므로 루프 FX 는 상태를 떠나도 계속 재생된다. 상태에 묶인 지속 효과를 저작할 방법이 현재 없다.
- **제안**: 기존 단발 효과 동작을 기본값으로 두고, 상태 이탈 시 `Deactivate` 하는 선택 옵션을 추가한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 11. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h:6`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs:27`
- **범주**: 설계/구조
- **문제**: Public 헤더가 `Components/StateTreeComponent.h`(GameplayStateTreeModule)를 포함하는데 그 모듈은 Private 의존성이다. 지금은 모듈 밖에서 이 헤더를 포함하는 곳이 없어 문제가 드러나지 않지만, 소비 모듈이 생기면 자기 쪽에 별도 의존성 선언이 없는 한 포함 경로가 보장되지 않는다. 클래스를 export 하지 않는 것 자체는 `Source/WxEditor/WxDeviceLinkVisualizer.h:12` 에 근거가 적혀 있어 의도된 선택으로 본다.
- **제안**: 외부 소비가 없다면 헤더를 Private 로 옮겨 배치와 의도를 맞춘다. 열 계획이 있다면 `GameplayStateTreeModule` 을 Public 의존성으로 승격한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_SpawnNiagara.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, 나머지 태스크·시스템 파일(`WxStateTreeTask_PlaySound`, `WxStateTreeTask_RecordCheckpoint`, `WxStateTreeTask_RespawnSpawners`, `WxStateTreeTask_ApplyGameplayEffectToInteractor`, `WxSpawnable`, `WxSpawnerLibrary`, `WxSpawnerLocatorUtils`, `WxDeviceComponentName`, `WxCheckpointSubsystem`, `WxWorldDeveloperSettings`, `WxWorldModule`)과 대응 헤더 전부. 소비 측 대조로 `Source/WxGame/Character/WxEnemyCharacter.cpp`(`OnSpawnedBy`·`MarkKilled` 호출부), `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp` 를 읽었다.
- **규칙 점검 결과**: 소스 57파일 전부 Copyright 첫 줄 존재, `FORCEINLINE`·헤더 인라인 정의 없음(`GetInstanceDataType()` 16건과 `TWxStateTreeWaitRegistry` 는 각 지점에 예외 사유 주석 있음), `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll` 한 건으로 BP Function Library 예외에 부합, 람다는 `WxInteractionScannerComponent.cpp:182` 의 정렬 술어 하나로 불가피, 델리게이트 콜백은 `HandleScanTimer`·`HandleBeginApplyTransition` 모두 `Handle` prefix, override 의 `Super::` 호출은 전부 존재(`FollowAuthorityState`/`RequestState` 가 `Super::StopLogic`·`Super::RestartLogic` 을 직접 부르는 것은 public 오버라이드의 재시도 초기화를 우회하려는 의도로 주석에 근거 있음). 의존성은 `*.uplugin` 이 `WxCore`·엔진 플러그인만, `Build.cs` 가 `WxCore` 만 참조하며 모듈 밖 Wx 헤더 포함도 `WxCore` 의 `WxInteractable.h`·`WxGameplayTags.h`·`WxLocatorUtils.h` 셋뿐이다. 위반 없음.
- **미검토 / 한계**: C++ 정적 검토이며 빌드·PIE 재현·StateTree/BP/WBP 에셋 검증은 하지 않았다. 1번은 엔진의 미해소 NetGUID 재통지 동작과 relevancy 설정에 의존하므로 2인 이상 PIE 로 확인이 필요하고, 4번은 실제 WP 셀 분할과 퀘스트 배치에 따라 체감이 갈린다. 5번(링크 에셋 태그)·7번(문·엘리베이터 조합)도 저작된 장치 ST 에셋이 그 구성을 쓰는지에 달려 있다. 스캐너의 `AllObjects` 오버랩(반경 150cm, 0.1초 주기)과 `FWxStateTreeTask_WaitForInteraction::IsAwaited` 의 스캔당 `SyncFind` 재해석은 비용을 재 보지 않았고 규모가 작다고 판단해 결과에 싣지 않았다. `IsAwaited`(`Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:37`)가 형제 함수와 달리 `Target` 널 검사 없이 역참조하는 점은 현 호출부가 `this` 하나뿐이라 제외했고, `WxStateTreeTask_RecordCheckpoint.cpp:43` 의 서브시스템 포인터 무검사 역참조도 `UWxCheckpointSubsystem` 이 항상 생성되므로 제외했다. `AWxDevice::BeginPlay` 가 부착 부모 장치를 `LinkedDevices` 에 암묵 추가하는 것(`Private/Device/WxDevice.cpp:128-134`)은 배선 의도를 숨기지만 의도된 편의로 보아 제외했다.

---
*문서 기준 커밋 `231068b` · 리뷰일 2026-09-13 · 소스 57파일 — `/module-review`로 갱신*
