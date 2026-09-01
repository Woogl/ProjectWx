# WxGame — 코드 리뷰

> 도메인 조립 글루로서 대체로 정돈된 모듈이다. Experience 파이프라인은 Lyra 이식이 충실하고, Tick 오버라이드가 한 곳도 없으며 코딩 규칙 준수도 높다. 다만 MVVM 뷰모델 계층은 나머지 코드보다 성숙도가 한 단계 낮고, 보스 체력바가 배치형 보스를 놓치는 실제 버그가 여기서 나왔다. 이번 리뷰는 `*.Build.cs`와 Framework(Experience 전 경로)·Character·Controller·AbilitySystem·Inventory·Cheat·Input·MVVM을 cpp까지 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 6 |

## 결과

### 1. 🔴 보스 체력바가 스트리밍-인 된 보스를 영영 잡지 못한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp:17` (시드는 `:19-23`)
- **범주**: 버그/정확성
- **문제**: VM 이 보스를 찾는 경로가 둘뿐인데 둘 다 배치형 보스를 놓친다. (a) `World->AddOnActorSpawnedHandler` 는 `UWorld::SpawnActor` 경로에서만 발화하고, 레벨 패키지에서 로드되는 액터는 이 경로를 지나지 않는다. (b) `StartObserving` 의 `TActorIterator` 시드는 VM 이 만들어지는 그 한 순간만 훑는다. 그런데 `BP_Boss` 는 스폰이 아니라 월드 파티션 배치 액터다 — `Content/__ExternalActors__/Maps/LV_OpenWorld/6/6J/E739HF09XZVERPTLGOXTLC.uasset` 과 `Content/__ExternalActors__/Maps/LV_DevCombat/C/NZ/9TD3LG9ZMG6AZNF70Z5B9R.uasset` 두 건이며, 코드베이스 어디에도 보스를 `SpawnActor` 하는 곳이 없다(스포너의 일반 스폰 경로에 보스 클래스를 꽂은 경우만 우연히 동작한다).
  실패 시나리오: HUD 는 빙의 시점에 푸시되므로 그때 먼 보스 셀은 아직 로드 전 → 시드 실패. 이후 플레이어가 보스 방에 들어가 셀이 스트리밍-인 돼도 `OnActorSpawned` 가 오지 않아 `SetBoss` 가 불리지 않는다 → 체력바가 끝까지 안 뜬다. 왕복은 더 나쁘다: 한 번 잡혔더라도 셀이 스트리밍-아웃되면 `OnEndPlay` → `SetBoss(nullptr)` 로 해제되고, 재검출 경로가 없어 다시 들어와도 **영구히** 복구되지 않는다. 좁은 `LV_DevCombat` 에선 시작 지점이 보스 셀 로딩 범위 안이라 시드가 우연히 성공해 그동안 드러나지 않았을 가능성이 크다.
- **제안**: `AWxBossCharacter::BeginPlay` 에서 정적 Ready 신호를 발행하고 VM 이 그걸 구독한다 — 이 코드베이스가 `UWxInventoryComponent::OnAnyInventoryReady`(`WxViewModel_Inventory.cpp:28`)와 `UWxInteractionScannerComponent::OnAnyScannerReady`(`WxViewModel_InteractionList.cpp:25`)로 이미 두 번 쓰고 있는 바로 그 패턴이고, 스트리밍 왕복에도 매번 다시 붙는다.
- **확신도**: 높음

### 2. 🟡 `OnUnregister` 가 리더 메시의 애님 틱 옵션을 되돌리지 않는다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.cpp:54-56`, `Source/WxGame/Character/WxMetaHumanComponent.cpp:129-132`
- **범주**: 버그/정확성
- **문제**: `OnRegister` 는 바디를 만들 때 리더 메시에 두 가지를 건다 — `SetVisibility(false)` 와 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones`. 그런데 `OnUnregister` 는 가시성만 되돌리고 틱 옵션은 그대로 둔다. `OnRegister` 는 `World->IsGameWorld()` 를 보지 않아 에디터 뷰포트에서도 도는데, 리더 메시(`ACharacter::Mesh`)는 트랜지언트가 아닌 네이티브 서브오브젝트라 이 비대칭이 레벨/BP 저장 시 굳을 여지가 있다. 가시성을 굳이 되돌리는 코드가 이미 있다는 것 자체가 그 위험을 인지한 설계인데, 짝이 되는 다른 한 줄이 빠졌다. 굳는 경우의 대가는 부착물이 없는 캐릭터도 화면 밖에서 매 프레임 본 리프레시를 계속 도는 것이다.
- **제안**: `OnRegister` 에서 덮기 전 값을 기억해 두고 `OnUnregister` 의 `SetVisibility(true)` 옆에서 함께 복원한다.
- **확신도**: 중간

### 3. 🟡 획득 토스트 뷰모델이 인벤토리 델리게이트에 물린 채 버려진다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:108-111`
- **범주**: 설계/구조 (오브젝트 수명)
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 새 `UWxViewModel_Item` 을 만들어 `Initialize(Inventory, ItemDef)` 를 부르는데, 그 `Initialize` 는 `WxViewModel_Item.cpp:45-46` 에서 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 두 델리게이트에 구독한다. 그런데 이 VM 은 `LastAcquiredItem` 한 칸에만 담기고 교체될 때 아무도 `Deinitialize()` 를 불러주지 않는다. `AllItems` 에 들어가지 않으므로 `RefreshAllItems` 의 정리 루프(`:152-158`)에도 걸리지 않는다. 아이템을 20번 주우면 버려진 토스트 VM 19개가 인벤토리 멀티캐스트에 살아남아 이후 모든 스택/충전 변경마다 19번씩 헛일한다. 약참조라 GC 가 돌면 조용히 정리돼 크래시는 아니지만, 그때까지 브로드캐스트 비용이 선형으로 쌓인다. 토스트는 스냅샷 표시인데 라이브 구독을 들고 있는 것 자체가 설계 착오다.
- **제안**: 교체 직전 이전 `LastAcquiredItem` 을 `Deinitialize()` 하거나, 더 낫게는 토스트용을 구독 없는 스냅샷 세터로 채운다.
- **확신도**: 중간

### 4. 🟡 스택 변경 한 번마다 아이템 목록 전체 재평가 + 열린 위젯 수만큼 곱하기
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:115` → `:118-165`, `:167-189`, 리졸버 `:212-224`
- **범주**: 성능/안전
- **문제**: 골드 1원이 들어와도 `HandleStackChanged` → `RefreshAllItems()` 가 목록 전체를 다시 돈다. 그 안에서 (a) `Inventory->GetAllItems()` 가 `TArray` 를 값으로 새로 만들고(`Plugins/WxInventory/.../WxInventoryComponent.h:196`), (b) `:133-140` 이 인스턴스마다 기존 VM 배열을 선형 탐색해 O(N×M) 이 되며, (c) `:162`·`:188` 이 `AllItems` 와 `CategorizedItems` 를 **무조건** 브로드캐스트해 열려 있는 ListView 엔트리 위젯이 전량 재생성된다. `:161` 주석은 "항상 브로드캐스트"를 정당화하지만 재평가까지 정당화하지는 않는다. 여기에 `UWxViewModelResolver_Inventory`(`:221`)가 위젯마다 별개 VM 을 만들어 각자 자기 자식 VM 트리와 구독을 갖는다 — 인벤토리 화면·통화 HUD·토스트가 동시에 떠 있으면 위 비용이 3배가 된다. 같은 폴더의 `WxViewModelResolver_PlayerCharacter.cpp:22` 는 `FindSharedViewModel` 로 공유하는데 여기만 안 한다.
- **제안**: `HandleStackChanged` 에서는 영향받은 ItemDef 슬롯만 증분 갱신하고 멤버십이 실제로 바뀐 경우에만 `RefreshAllItems` 를 부른다. 리졸버는 `FindSharedViewModel(Inventory, ...)` 로 인벤토리당 하나를 공유한다.
- **확신도**: 중간

### 5. 🟡 클라이언트에서 `InitAbilitySystem` 이 도는 경로가 플레이어 폰 하나뿐이다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:110-115`, `Source/WxGame/Character/WxCharacterBase.cpp:223-246`, `Source/WxGame/Character/WxPlayerCharacter.cpp:75-80`
- **범주**: 설계/구조 (권위 모델)
- **문제**: `InitAbilitySystem()` 의 호출 지점은 저장소 전체에서 딱 둘이다 — 서버 전용인 `AWxCharacterBase::PossessedBy` 와 소유 클라의 `AWxPlayerCharacter::OnRep_PlayerState`. 여기서 둘이 갈라진다. (a) `AWxEnemyCharacter`·`AWxMinion` 의 시뮬 프록시는 클라에서 이 함수를 한 번도 타지 않아 SPD→`MaxWalkSpeed` 구독(같은 파일 `:230-239`)이 설치되지 않는다 — 원격 클라 화면에서 적의 이동 속도가 SPD 배율을 반영하지 못한다. (b) 플레이어 폰조차 `OnRep_Controller` 경로에는 재초기화가 없어, `OnRep_PlayerState` 가 먼저 도착하면 `RefreshAbilityActorInfo()` 가 `Pawn->GetController()` 를 널로 읽어 `AbilityActorInfo->PlayerController` 가 빈 채로 남는다. 헤더 `WxCharacterBase.h:111-113` 이 "클라이언트: 파생 클래스에서 OnRep을 통해 호출"이라 못박고 있어 의도된 단순화로 보이지만, 멀티 검증 시 가장 먼저 깨질 지점이다.
- **제안**: 지금 고칠 일은 아니고, 멀티 대응 착수 시 체크리스트로 쓴다 — `NotifyControllerChanged` 에도 `InitAbilitySystem()` 을 걸고 AI 폰용 클라 초기화 훅을 `AWxCharacterBase` 에 하나 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 관찰자 3종 세트와 자식 VM 재구축이 통째로 복붙돼 있다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:12-29`·`:191-210` ↔ `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:9-26`·`:101-120`; `Source/WxGame/MVVM/WxViewModel_Quest.cpp:55-66` ↔ `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:122-133`
- **범주**: 중복/복잡도
- **문제**: `StartObserving`/`Handle~Ready`/`StopObserving` 삼총사가 타입 이름만 바꾼 채 두 벌 존재하며, 주석 문구("신호는 클래스 차원이라 남의 …도 온다")까지 동일하다. 1번 결함을 제안대로 고치면 보스 VM 이 세 번째 사본이 된다. 자식 VM 목록 재구축도 두 벌인데(`RebuildObjectives` ↔ `RebuildEntries`), 둘 다 `Reset` → 루프 `NewObject` + 세터 → 브로드캐스트 형태라 텍스트만 바뀌어도 자식 UObject 를 전량 새로 만든다.
- **제안**: 1번을 고치기 전에 먼저 정리한다 — `UWxViewModel` 에 "정적 Ready 신호를 관찰하다 소유 액터로 필터해 연결"하는 헬퍼와, 개수가 같으면 세터만 부르는 자식 VM 동기화 헬퍼를 올린다.
- **확신도**: 높음

### 7. 🟢 입력 바인딩 콜백에 `Handle` 접두사가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:109`, `:113`, `:122`, `:127`, `:128`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. `EIC->BindAction(...)` 에 물린 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 가 모두 벗어나 있다. 모듈의 나머지 27개 `AddDynamic`/`AddUObject`/`CreateUObject` 바인딩은 전부 규칙을 지키고 있어 이 함수 하나만 예외로 남은 모양새다.
- **제안**: 위 다섯 개를 `HandleMove`·`HandleLook`·`HandleToggleCrouch`·`HandleAbilityInputTriggered`·`HandleAbilityInputReleased` 로 개명한다. 같은 함수 `:117`·`:118` 에 물린 `AWxPlayerCharacter::Jump`(`ACharacter` 가상 오버라이드)와 `ACharacter::StopJumping`(엔진 함수)은 개명 대상이 아니다.
- **확신도**: 높음

### 8. 🟢 `Request~` 규약 밖의 `BlueprintCallable`
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.h:80-81`
- **범주**: 규칙 위반
- **문제**: `UWxViewModel_Inventory::SetCurrentCategory` 가 `BlueprintCallable` 인데, 코딩 규칙 5(BP Function Library·Async 팩토리 한정)와 이 프로젝트가 예외로 인정한 VM Command(`Request~`) 어느 쪽에도 들지 않는다. 하는 일(카테고리 설정 → 목록 갱신)은 형제인 `RequestAdvance`·`RequestInteract`·`RequestCycle`·`RequestUseConsumable` 과 같은 VM Command 인데 이름만 규약을 벗어났다.
- **제안**: `RequestSetCategory` 로 개명해 다른 VM Command 와 어휘를 맞춘다. 위젯 바인딩이 함수명을 참조하므로 개명 후 해당 WBP 의 MVVM 이벤트 바인딩을 함께 갱신해야 한다.
- **확신도**: 높음

### 9. 🟢 `Deinitialize` 의 비우기 브로드캐스트가 VM 마다 제각각이다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:56-60`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp:33-37`
- **범주**: 설계/구조
- **문제**: `Deinitialize()` 는 재초기화 경로뿐 아니라 `Plugins/WxUI/.../WxViewModel.cpp:33` 의 `UWxViewModel::BeginDestroy()` 에서도 불린다. `BeginDestroy` 는 GC 가 이 VM 을 unreachable 로 판정한 뒤에 도는데, 그때 VM 을 붙들던 `UMVVMView`/위젯도 같이 unreachable 이라 반쯤 해체된 뷰의 바인딩으로 브로드캐스트가 들어갈 수 있다. 같은 폴더의 `WxViewModel_Inventory.cpp` 와 `WxViewModel_Item.cpp` 는 같은 상황에서 브로드캐스트를 하지 않아, 다섯 VM 의 teardown 규약이 서로 다르다.
- **제안**: 비우기 브로드캐스트를 재초기화 경로(`Initialize` 진입부)로 옮기고 `Deinitialize` 에서는 값만 리셋해 규약을 하나로 맞춘다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 10. 🟢 `BaseWalkSpeed` 가 초기화자 없이 선언돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:145`
- **범주**: 성능/안전
- **문제**: `float BaseWalkSpeed;` 에 초기화자가 없다. 현재는 대입(`WxCharacterBase.cpp:234`)이 구독 설치보다 반드시 먼저 일어나 쓰레기 값을 읽는 경로가 없지만, `HandleSPDAttributeChanged`(같은 파일 `:250`)가 이 값을 그대로 `MaxWalkSpeed` 에 곱해 넣으므로 초기화 순서가 한 번만 흐트러져도 즉시 눈에 보이는 이동 속도 이상으로 드러난다. 같은 헤더의 다른 멤버는 모두 초기값을 갖고 있어 이 하나만 예외다.
- **제안**: `float BaseWalkSpeed = 0.f;`
- **확신도**: 높음

### 11. 🟢 공개 include 경로의 헤더가 Private 의존 모듈 헤더를 포함한다
- **위치**: `Source/WxGame/WxGame.Build.cs:11`·`:38-43`, `Source/WxGame/Character/WxMetaHumanComponent.h:7`
- **범주**: 설계/구조
- **문제**: `PublicIncludePaths` 에 `ModuleDirectory` 를 통째로 넣어 모듈의 모든 헤더를 다운스트림에 노출하는데, `MetaHumanSDKRuntime` 은 `PrivateDependencyModuleNames` 에 있다. 그 결과 `WxMetaHumanComponent.h` 가 포함하는 `MetaHumanComponentUE.h` 의 include 경로가 다운스트림으로 전파되지 않는다. 지금은 이 헤더를 WxGame 내부 cpp 세 곳만 포함해 드러나지 않지만, `WxEditor` 나 앞으로 만들 `Plugins/GameFeatures/` 콘텐츠 플러그인(규칙상 WxGame 참조 허용)이 캐릭터 헤더를 건드리는 순간 컴파일이 깨진다.
- **제안**: `MetaHumanSDKRuntime` 을 `PublicDependencyModuleNames` 로 올리거나, `UWxMetaHumanComponent` 를 공개 표면에서 뺀다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/MVVM/` 전 파일 (짝이 되는 헤더 포함)
- **훑은 파일**: `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/Character/WxMinion.cpp`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/WxGame.cpp`
- **확인했으나 지적하지 않은 것**: 코딩 규칙 1(Wx 접두사)·2(저작권 첫 줄)·3(람다 — 모듈 전체 0건)·6(인라인 정의 — 0건)·7(`Super::` 누락 — override 82개 전수 확인, 누락 없음)은 전부 깨끗하다. `WxCharacterBase.cpp` 등 7개 파일의 UTF-8 BOM 은 편집기에서 보이지 않아 제외했다. `UWxItemUseComponent` 는 구독·해제 짝과 재진입 가드(`:86-87`)가 모두 정확해 결함이 없다. `AWxEnemyCharacter::HandleDeath` 가 항상 0번 플레이어에게 보상을 주는 것, `UWxViewModelResolver_BossCharacter` 가 위젯별 관찰형 VM 을 만드는 것은 주석·기존 결정으로 의도가 명시돼 있다. `/Game/Framework` Experience 스캔 설정(`Config/DefaultGame.ini:48-49`)은 코드의 전제와 일치한다. `UWxViewModel_Item` 의 아이콘 이중 요청과 `HandleStackChanged` 중 델리게이트 재진입은 각각 베이스의 `CancelHandle` 과 UE 멀티캐스트의 지연 컴팩션으로 안전함을 확인했다.
- **미검토 / 한계**: `UWxExperienceManagerComponent` 의 실패·중도 종료 경로는 코드상 정합하나 실제 GameFeature 활성 실패를 재현해 확인하지는 않았다. 1번 결함은 정적 분석과 에셋 배치로 확정했을 뿐 PIE 재현은 하지 않았다 — 수정 전 `LV_OpenWorld` 에서 먼저 재현해 보길 권한다. 범위 밖이지만 `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp:77-85` 는 주석("자리를 빈 텍스트로 채운다")과 코드(`if (AActor* Actor = Weak.Get())` 로 자리를 건너뜀)가 어긋나 있고, `WxViewModel_InteractionList.cpp:43-44` 의 초기 시드가 이를 그대로 읽어 프롬프트와 선택 인덱스가 일시적으로 어긋날 수 있다 — WxWorld 리뷰 때 확인할 것. BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 70파일 — `/module-review`로 갱신*
