# WxInventory — 코드 리뷰

> 모듈 코드 품질은 계속 좋다. 모든 변경 진입점에 권위 게이팅이 걸려 있고, 서버 변경 경로와 클라 복제 콜백이 같은 `Notify*` 진입점으로 수렴하는 구조도 일관되며, 프로젝트 코딩 규칙 위반은 0건이다. **심각(🔴) 등급은 없다.** 지난 리뷰(`1e9b745c`)의 🟡 "픽업 이동 복제 누락" 은 `WxItemPickup.cpp:24` 의 `SetReplicateMovement(true)` 로 **해결되어 제외**했고, 대신 같은 파일에서 새 항목 1건(픽업 Niagara 의 서버 경로 무효화)을 찾았다. 이번 리뷰는 소스 22개를 모두 열고 `WxInventoryManagerComponent.cpp`·`WxItemPickup.cpp`·`WxEquipmentComponent.cpp`·`WxRewardLibrary.cpp`·StateTree Task 2종을 라인 단위로 본 뒤, 엔진 5.8 소스(`NiagaraComponent.cpp`·`Actor.cpp`·`ActorComponent.cpp`), 호출부(`Source/WxGame` 의 UseItem 어빌리티·GameMode·EnemyCharacter·뷰모델), 에셋 덤프(`BP_ItemPickup`)로 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 보상 지급·충전 리필 대상이 0번 PlayerController 로 고정돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp:34`
- **범주**: 설계/구조
- **문제**: 두 StateTree Task 모두 `Owner->HasAuthority()` 로 권위를 확인한 직후 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 정한다. 주석은 이를 "로컬 플레이어(0번 컨트롤러)" 라 부르지만 권위 측(데디케이티드 서버)에는 로컬 플레이어가 없고 0번은 접속 순서에 따른 임의의 플레이어다. 플레이어 2가 상자를 열면 재화가 플레이어 1에게 들어가고, 플레이어 2가 체크포인트를 밟으면 플레이어 1의 에스트병이 채워진다. 발동 주체 정보는 `IWxInteractable::OnInteracted(Interactor, Source)` 로 이미 권위 측에 도달해 있으므로 없는 정보가 아니다.
- **제안**: 두 Task 의 InstanceData 에 대상 Actor 바인딩을 추가해 기믹이 들고 있는 instigator 를 꽂고, 비었을 때만 현재의 0번 PC 폴백을 유지한다. 멀티 정책 확정 전까지 손대지 않을 거라면 최소한 주석의 "로컬 플레이어" 표현을 "권위 측 0번 컨트롤러(단일 플레이 전제)" 로 정정한다.
- **확신도**: 중간 (싱글플레이 범위를 전제한 의도적 단순화일 수 있음 — 주석이 그 전제를 두 곳에서 명시한다)

### 2. 🟡 픽업의 Niagara 활성/비활성 지정이 서버 스폰 경로에서 통째로 무시된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:140-148` (`ApplyPickupVisual` 의 Niagara 블록), 진입은 `:56`, 호출 순서는 `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:76`·`:82`·`:83`
- **범주**: 버그/정확성
- **문제**: `GrantReward` 는 `SpawnActorDeferred`(`:76`) → `SetItemDef`(`:82`) → `FinishSpawning`(`:83`) 순서로 스폰하므로, `SetItemDef` 가 부르는 `ApplyPickupVisual` 시점에는 컴포넌트가 아직 **등록 전**이다. UE 5.8 `UNiagaraComponent::ActivateInternal` 은 초입에서 `if (!IsRegistered()) { return; }` 로 빠져나가고(`NiagaraComponent.cpp`), `Deactivate` 도 같은 시점엔 활성 플래그를 내리는 것 말고 하는 일이 없다. 그 뒤 `AActor::InitializeComponents`(`Actor.cpp`)가 `bAutoActivate && !IsActive()` 인 컴포넌트를 일괄 `Activate(true)` 하는데, `UNiagaraComponent` 는 생성자에서 `bAutoActivate = true` 다. 결과적으로 **Pickup Fragment 에 `NiagaraSystem` 을 지정하지 않아도 픽업 BP 에 박힌 기본 이펙트가 그대로 재생된다** — `BP_ItemPickup` 은 `NiagaraComponent.asset = NS_Loot_Drop_3`, `bAutoActivate = true` 로 실제 그 상태다(`.claude/asset_dump/Blueprints/BP_ItemPickup.json`). 반대로 리모트 클라는 `OnRep_ItemDef`(`:117`) 경로라 컴포넌트가 이미 등록돼 있어 `Deactivate()` 가 정상 동작한다. 즉 서버/스탠드얼론에서는 이펙트가 보이고 리모트 클라에서는 안 보이는 불일치이며, 현재 주력인 싱글플레이에서는 "Fragment 미지정 = 이펙트 없음" 이라는 의도가 항상 깨진다.
- **제안**: 픽업 비주얼 적용을 등록 이후 시점(`BeginPlay` 또는 `PostInitializeComponents`)으로 옮기거나, `NiagaraComponent` 의 `bAutoActivate` 를 생성자에서 false 로 두고 `ApplyPickupVisual` 이 활성 여부를 단독으로 소유하게 한다. 후자가 데이터 소유권이 더 명확하다.
- **확신도**: 높음 (엔진 5.8 소스와 BP 덤프로 양쪽 확인)

### 3. 🟡 장비 경로가 트리거 없이 전 캐릭터에 상주한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:609` (`EquipItemByDef`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34` (`EquipItem`)
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 의 호출부가 저장소 전체에 0건이고, `UWxEquipmentComponent::EquipItem` 은 그 `EquipItemByDef`(`:635`) 에서만 불린다. 둘 다 `BlueprintCallable` 이 아니라 BP 진입도 없다. 헤더가 이 상태를 이미 명시하지만(`Public/Inventory/WxEquipmentComponent.h:29-33`), 그 사이 컴포넌트는 `AWxCharacterBase` 생성자가 전 캐릭터(플레이어·적·보스)에 무조건 부착하고(`Source/WxGame/Character/WxCharacterBase.cpp:40`) 캐릭터가 `OnEquipVisualChanged` 를 구독까지 한다(`:98`). 즉 모든 폰이 값이 절대 바뀌지 않는 복제 프로퍼티와 컴포넌트 등록 비용을 계속 내면서, 실행된 적 없는 코드라 GE 스왑 순서·해제 타이밍의 정확성은 한 번도 검증되지 않았다. `RemoveItemInstance`(`WxInventoryManagerComponent.cpp:364`) 도 호출부 0건이다.
- **제안**: 트리거(UI 슬롯 → 어빌리티/서버 RPC → `EquipItemByDef`)를 붙여 경로를 닫거나, 당분간 쓸 계획이 없으면 부착과 구독을 걷어내 되살릴 때 다시 붙인다. 경로를 닫을 때는 헤더 `:33` 이 예고한 pull 공백도 함께 처리해야 한다 — 외형 반영이 `OnEquipVisualChanged` 방송 한 번에만 의존해서, 이미 장비를 착용한 폰이 뒤늦게 relevant 해진 클라에서는 `OnRep_EquippedItemDef` 가 구독보다 먼저 돌아 방송이 유실되고 되물을 API 가 없다(`GetEquippedItemDef()` 또는 `RefreshEquipVisual()` 노출).
- **확신도**: 높음

### 4. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:288`, `:324-337`
- **범주**: 성능/안전
- **문제**: `const int32 MaxStack = Stackable ? Stackable->MaxStack : 1;` 로 받은 값을 `while (Remaining > 0) { const int32 ChunkCount = FMath::Min(MaxStack, Remaining); ... Remaining -= ChunkCount; }` 로 쓴다. `MaxStack` 이 0 이하이면 `ChunkCount` 가 0 이하가 되어 `Remaining` 이 줄지 않고, 매 회 `AddEntry` 로 엔트리를 만들며 무한 루프 → 에디터 행/OOM 이다(에러 로그 없이 멈춰 원인 파악이 오래 걸리는 유형). `UWxItemFragment_Stackable::MaxStack` 의 `meta = (ClampMin = "1")`(`Public/Items/WxItemFragment.h:144`)은 디테일 패널 입력만 막고 이전에 직렬화된 값까지 되돌리지는 않는다.
- **제안**: `MaxStack` 을 읽는 `:288` 한 지점에서 `FMath::Max(1, ...)` 로 하한을 강제한다. 루프 자체를 방어할 필요는 없다.
- **확신도**: 중간 (정상 편집 경로는 ClampMin 이 막고 있어 데이터를 신뢰하는 설계일 수 있음)

### 5. 🟢 소비되는 아이템 인스턴스를 GE SourceObject 로 넘긴 뒤 그 인스턴스를 파괴한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:555` (`Context.AddSourceObject`), `:573` (`ConsumeItemsByDefinition`), `:580` (`ApplyGameplayEffectSpecToSelf`)
- **범주**: 버그/정확성
- **문제**: 비충전형(Charges Fragment 없음) 경로에서는 `ConsumeItemsByDefinition` 이 마지막 1개를 차감하며 엔트리를 제거해 `UWxItemInstance` 를 고아로 만든다. 그 직후 적용되는 GE 의 `SourceObject` 는 그 인스턴스를 가리키는데 `FGameplayEffectContext::SourceObject` 는 weak 포인터라 다음 GC 이후 null 이 된다. Instant GE 라면 같은 프레임에 끝나므로 무해하지만, Duration/Infinite GE 가 붙거나 주석(`:553`)이 말하는 "인스턴스 단위 데이터 추적" 을 실제로 하려는 소비처(ExecutionCalculation·UI 등)가 생기면 조용히 null 을 본다. 충전형 경로는 인스턴스가 남으므로 영향 없다.
- **제안**: 소비형은 SourceObject 를 인스턴스 대신 `UWxItemDefinition`(정적 자산이라 수명 안전)으로 넘기거나, 인스턴스 추적이 꼭 필요하면 GE 적용을 차감보다 먼저 수행한다.
- **확신도**: 낮음 (현재 소비처가 Instant GE 뿐이라면 의도된 설계일 수 있음)

### 6. 🟢 데디케이티드 서버에서도 픽업 시각 에셋을 동기 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:135`, `:140`
- **범주**: 성능/안전
- **문제**: `ApplyPickupVisual` 은 `OnRep_ItemDef`(클라)뿐 아니라 서버 전용 진입점인 `SetItemDef`(`UWxRewardLibrary::GrantReward` 가 호출) 에서도 실행돼 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous` 한다. 물리 시뮬레이션 때문에 서버가 StaticMesh 를 필요로 하는 것은 맞지만 NiagaraSystem 은 순수 코스메틱이고, 엔진도 데디케이티드 서버에서는 `ActivateInternal` 초입에서 `World->IsNetMode(NM_DedicatedServer)` 로 걸러 재생 자체를 하지 않는다 — 즉 로드분이 전부 낭비다. 드랍마다 불필요한 동기 로드 히치와 상주 메모리가 생기고 드랍 종류가 늘수록 누적된다.
- **제안**: Niagara 블록만 `IsNetMode(NM_DedicatedServer)` 로 건너뛴다(#2 의 수정과 같은 지점이라 함께 처리하면 된다). 지급·프롬프트 로직은 시각 에셋에 의존하지 않는다.
- **확신도**: 높음 (데디케이티드 서버를 운영할 때에 한해 영향)

### 7. 🟢 uplugin 이 Niagara 의존을 선언하지 않는다
- **위치**: `Plugins/WxInventory/WxInventory.uplugin:16`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs:26`
- **범주**: 설계/구조
- **문제**: Build.cs 가 `Niagara` 를 PrivateDependency 로 쓰고 `AWxItemPickup` 이 `UNiagaraComponent`/`UNiagaraSystem` 을 직접 참조하는데, uplugin 의 `Plugins` 배열에는 `WxCore`·`GameplayAbilities`·`ModularGameplay`·`StateTree` 만 있고 Niagara 가 빠져 있다. 지금은 Niagara 가 엔진 기본 활성이라 통하지만, 다른 세 엔진 플러그인은 전부 선언한 것과 어긋나고 비활성 환경에서는 플러그인 의존성 오류 대신 컴파일 오류로 드러난다.
- **제안**: uplugin 의 `Plugins` 배열에 `Niagara` 를 추가한다.
- **확신도**: 높음

### 8. 🟢 `GrantReward` 가 `UWorld` 를 널 검증 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:39` (획득), `:76` (`World->SpawnActorDeferred`)
- **범주**: 버그/정확성
- **문제**: 임의의 `AActor*` 를 받는 public static `BlueprintCallable` 진입점인데 `SourceActor->GetWorld()` 결과를 검사하지 않는다. `HasAuthority()` 는 `ROLE_Authority` 검사라 월드에 속하지 않은 객체도 통과할 수 있고, 그 경우 픽업 보상 항목에서 널 역참조로 크래시한다. 모듈의 다른 널 경로는 전부 가드돼 있어 이 한 곳만 비어 있다.
- **제안**: 한 줄 조기 반환을 넣거나, 저ROI 가드로 판단해 넣지 않을 거라면 그 판단을 주석으로 남긴다 — 근거 없이 두면 리뷰마다 같은 항목이 다시 올라온다(이번이 다섯 번째 지적이다).
- **확신도**: 낮음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, 경계 확인용 외부 호출부(`Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `WxViewModel_Inventory.cpp`)
- **미검토 / 한계**: 런타임 검증(PIE 2인 이상 네트워크 테스트)은 하지 않았다. #1·#2 는 엔진 소스와 에셋 덤프 기반 정적 판단이라 실제 재현은 남아 있다. BP/WBP 내부와 ItemDefinition 데이터 자산의 Fragment 조합 타당성은 범위 밖이다. 반대로 이번에 확인해 **문제없다고 판단한 것**은 다음 세션이 중복 조사하지 않도록 남긴다 — (a) **서브오브젝트 복제는 정상이다(과거 🔴 오탐, 재조사 금지)**: 컴포넌트만 `bReplicateUsingRegisteredSubObjectList`(`WxInventoryManagerComponent.cpp:228`)를 켜고 소유 `AWxPlayerController` 는 끈 조합을 **엔진이 정확히 지원한다**. 레거시 경로 `AActor::ReplicateSubobjects`(`ActorReplication.cpp`)가 각 복제 컴포넌트에 대해 `Channel->ReplicateSubobject(ActorComp, ...)` 를 부르고, 그 오버로드(`DataChannel.cpp`)의 `checkf(Actor->IsUsingRegisteredSubObjectList() == false, ...)` 가 **액터가 플래그를 끈 상태를 이 분기의 전제**로 못박는다. 컨트롤러에 플래그를 켜거나 `ReplicateSubobjects` 를 오버라이드할 필요가 **없다**. (b) **FastArray 참조 해결 순서도 안전하다**: `UActorChannel::ReplicateRegisteredSubObjects` 가 컴포넌트의 서브오브젝트를 컴포넌트 프로퍼티보다 **먼저** 쓰므로 `PostReplicatedAdd` 시점에 `Entry.Instance` 는 유효하다. (c) 모듈 경계: Build.cs/uplugin 의 Wx 의존은 `WxCore` 뿐이고 소스 내 비-WxInventory Wx 인클루드도 `WxInteractable.h`/`WxGameplayTags.h`/`WxCollisionChannels.h`(전부 WxCore) 뿐이다. (d) **규칙 준수 0건 위반**: 22개 소스 전부 첫 줄 저작권 표기 존재, 람다·`FORCEINLINE` 0건, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 한 곳뿐, Wx prefix 누락 없음, `Super::` 호출 누락 없음, 델리게이트 바인딩 자체가 모듈 내에 없어 `Handle` 규칙 대상 없음. 헤더 인라인은 템플릿 `FindFragmentByClass<T>()` 와 StateTree 관용 `GetInstanceDataType()` 뿐이며 둘 다 예외 사유가 주석에 있다. (e) FastArray 정합성: `AddEntry`/`RemoveEntry`/`AddToEntryStack`/`ConsumeByDefinition` 의 `MarkItemDirty`/`MarkArrayDirty` 조합, `CreateIterator`+`RemoveCurrent` 순회, `LastObservedCount` 의 `NotReplicated` 모두 올바르다. `AddItemDefinition` 의 머지 루프가 `GetEntries()` 참조를 통지 방송 너머로 들고 있지만 TArray 객체 자체는 이동하지 않고 매 회 재인덱싱하므로 재할당에 안전하다. (f) 통지 수렴: 클라 `PreReplicatedRemove` 가 제거 대상 `StackCount` 를 0으로 내린 뒤 총량을 재계산하므로 서버/클라 최종 합계가 일치한다. (g) 권위: `AddItemDefinition`/`ConsumeItemsByDefinition`/`UseItemByDef`/`RefillItemCharges` 의 `check(HasAuthority())` 는 모든 호출 경로(서버 전용 `OnInteracted`, `WxAbility_UseItem::HandleConsumeEvent` 의 `HasAuthority` 게이트, `AWxGameMode` 의 서버 지급, 권위 게이트된 StateTree Task)에서 안전하다. (h) 정적 델리게이트 `OnAnyInventoryReady` 는 구독자(`UWxViewModel_Inventory`)가 `AddUObject` + 명시 `Remove` 로 수명을 닫고 있어 누수·댕글링이 없다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 22파일 — `/module-review`로 갱신*
