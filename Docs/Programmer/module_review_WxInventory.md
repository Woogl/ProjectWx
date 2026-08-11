# WxInventory — 코드 리뷰

> 모듈 건강도는 여전히 좋다. 모든 변경 진입점에 권위 게이팅이 걸려 있고, 서버 변경 경로와 클라 복제 콜백이 같은 `Notify*` 진입점으로 수렴하는 구조가 일관되며, 프로젝트 코딩·모듈 규칙 위반은 기계 검사 기준 0건이다. **심각(🔴)은 없다.** 이번 리뷰는 소스 22개를 모두 열고 `WxInventoryManagerComponent.cpp`·`WxItemPickup.cpp`·`WxEquipmentComponent.cpp`·`WxRewardLibrary.cpp`·StateTree Task 2종을 라인 단위로 본 뒤, UE 5.8 엔진 소스(`NiagaraComponent.cpp`·`Actor.cpp`·`ActorReplication.cpp`)와 외부 호출부(`Source/WxGame` 의 UseItem 어빌리티·캐릭터·뷰모델)로 교차 검증했다. 지난 리뷰(`95a57ef3`)의 8건은 전부 현재 코드에서 재확인되어 유지했고, 신규 1건(#7)을 추가했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 보상 지급·충전 리필 대상이 0번 PlayerController 로 고정돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:36`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp:32`
- **범주**: 설계/구조
- **문제**: 두 StateTree Task 모두 `Owner->HasAuthority()` 로 권위를 확인한 직후 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 정한다. 주석은 이를 "로컬 플레이어(0번 컨트롤러)" 라 부르지만 데디케이티드 서버에는 로컬 플레이어가 없고 0번은 접속 순서에 따른 임의의 플레이어다. 플레이어 2가 상자를 열면 재화가 플레이어 1에게 들어가고, 플레이어 2가 체크포인트를 밟으면 플레이어 1의 에스트병이 채워진다. 발동 주체는 `IWxInteractable::OnInteracted(Interactor, Source)` 로 이미 권위 측에 도달해 있으므로 없는 정보가 아니다.
- **제안**: 두 Task 의 InstanceData 에 대상 Actor 바인딩을 추가해 기믹이 쥔 instigator 를 꽂고, 비었을 때만 현재의 0번 PC 폴백을 남긴다. 멀티 정책 확정 전까지 손대지 않을 거면 최소한 주석의 "로컬 플레이어" 를 "권위 측 0번 컨트롤러(단일 플레이 전제)" 로 정정한다.
- **확신도**: 중간 (싱글플레이 범위를 전제한 의도적 단순화일 수 있음 — 주석이 그 전제를 두 곳에서 명시한다)

### 2. 🟡 픽업 Niagara 의 활성/비활성 지정이 서버 스폰 경로에서 통째로 무시된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:140-148` (`ApplyPickupVisual` 의 Niagara 블록), 진입은 `:56`, 호출 순서는 `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:70`·`:76`·`:77`
- **범주**: 버그/정확성
- **문제**: `GrantReward` 는 `SpawnActorDeferred`(`:70`) → `SetItemDef`(`:76`) → `FinishSpawning`(`:77`) 순서로 스폰하므로, `SetItemDef` 가 부르는 `ApplyPickupVisual` 시점에는 컴포넌트가 아직 **등록 전**이다(등록은 `FinishSpawning` → `PostActorConstruction` 에서 일어난다). UE 5.8 `UNiagaraComponent::ActivateInternal` 은 초입에서 `if (!IsRegistered()) { return; }` 로 빠져나가고(`NiagaraComponent.cpp:1335`), `Deactivate` 도 이 시점엔 활성 플래그를 내리는 것 외에 하는 일이 없다. 그 뒤 `AActor::InitializeComponents`(`Actor.cpp:6399`)가 `bAutoActivate && !IsActive()` 인 컴포넌트를 일괄 `Activate(true)` 하는데 `UNiagaraComponent` 는 생성자에서 `bAutoActivate = true`(`NiagaraComponent.cpp:685`)다. 결과적으로 **Pickup Fragment 에 `NiagaraSystem` 을 지정하지 않아도 픽업 BP 에 박힌 기본 이펙트가 그대로 재생된다** — `else` 분기의 `Deactivate()` 가 무효화된다. 반대로 리모트 클라는 `OnRep_ItemDef`(`:117`) 경로라 컴포넌트가 이미 등록돼 있어 정상 동작하므로 서버/스탠드얼론과 클라의 표현이 갈린다.
- **제안**: 비주얼 적용을 등록 이후 시점(`BeginPlay`/`PostInitializeComponents`)으로 옮기거나, 생성자에서 `NiagaraComponent->bAutoActivate = false` 로 두고 `ApplyPickupVisual` 이 활성 여부를 단독으로 소유하게 한다. 후자가 데이터 소유권이 명확하다. 손대는 김에 #6 도 같은 블록에서 함께 처리하면 된다.
- **확신도**: 높음 (엔진 5.8 소스로 재확인. 단, 실제 증상 크기는 픽업 BP 의 `NiagaraComponent` 기본 에셋 지정 여부에 달렸고 이번 패스에선 BP 덤프가 없어 확인하지 못했다)

### 3. 🟡 장비 경로가 트리거 없이 전 캐릭터에 상주한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:602` (`EquipItemByDef`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34` (`EquipItem`)
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 의 호출부가 저장소 전체에 0건이고 `UWxEquipmentComponent::EquipItem` 은 그 `EquipItemByDef`(`:628`) 에서만 불린다. 둘 다 `BlueprintCallable` 이 아니라 BP 진입도 없다. 헤더가 이 상태를 명시하지만(`Public/Inventory/WxEquipmentComponent.h:28-31`), 그 사이 컴포넌트는 `AWxCharacterBase` 생성자가 전 캐릭터(플레이어·적·보스)에 무조건 부착하고(`Source/WxGame/Character/WxCharacterBase.cpp:37`) 캐릭터가 `OnEquipVisualChanged` 를 구독까지 한다(`:82`). 모든 폰이 값이 절대 바뀌지 않는 복제 프로퍼티와 컴포넌트 등록 비용을 계속 내면서, 실행된 적 없는 코드라 GE 스왑 순서·해제 타이밍의 정확성은 한 번도 검증되지 않았다. `RemoveItemInstance`(`WxInventoryManagerComponent.cpp:358`) 도 호출부 0건이다.
- **제안**: 트리거(UI 슬롯 → 어빌리티/서버 RPC → `EquipItemByDef`)를 붙여 경로를 닫거나, 당분간 계획이 없으면 부착·구독을 걷어내고 되살릴 때 다시 붙인다. 경로를 닫을 때는 헤더 `:31` 이 예고한 pull 공백도 함께 처리해야 한다 — 외형 반영이 `OnEquipVisualChanged` 방송 한 번에만 의존해서, 이미 장비를 착용한 폰이 뒤늦게 relevant 해진 클라에서는 `OnRep_EquippedItemDef` 가 구독보다 먼저 돌아 방송이 유실되고 되물을 API 가 없다(`GetEquippedItemDef()` 또는 `RefreshEquipVisual()` 노출).
- **확신도**: 높음

### 4. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:284`, `:318-336`
- **범주**: 성능/안전
- **문제**: `const int32 MaxStack = Stackable ? Stackable->MaxStack : 1;` 로 받은 값을 `while (Remaining > 0) { const int32 ChunkCount = FMath::Min(MaxStack, Remaining); ... Remaining -= ChunkCount; }` 로 쓴다. `MaxStack` 이 0 이하면 `ChunkCount` 가 0 이하가 되어 `Remaining` 이 줄지 않고, 매 회 `AddEntry` 로 엔트리를 만들며 무한 루프 → 에디터 행/OOM 이다(에러 로그 없이 멈춰 원인 파악이 오래 걸리는 유형). `UWxItemFragment_Stackable::MaxStack` 의 `meta = (ClampMin = "1")`(`Public/Items/WxItemFragment.h:145`)은 디테일 패널 입력만 막고 이미 직렬화된 값이나 C++/BP 서브클래스 기본값까지 되돌리지는 않는다.
- **제안**: `:284` 한 지점에서 `FMath::Max(1, ...)` 로 하한을 강제한다. 루프 자체를 방어할 필요는 없다.
- **확신도**: 중간 (정상 편집 경로는 ClampMin 이 막고 있어 데이터를 신뢰하는 설계일 수 있음)

### 5. 🟢 소비되는 아이템 인스턴스를 GE SourceObject 로 넘긴 뒤 그 인스턴스를 파괴한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:548` (`Context.AddSourceObject`), `:566` (`ConsumeItemsByDefinition`), `:573` (`ApplyGameplayEffectSpecToSelf`)
- **범주**: 버그/정확성
- **문제**: 비충전형(Charges Fragment 없음) 경로에서는 `ConsumeItemsByDefinition` 이 마지막 1개를 차감하며 엔트리를 제거해 `UWxItemInstance` 를 고아로 만든다. 그 직후 적용되는 GE 의 `SourceObject` 는 그 인스턴스를 가리키는데 `FGameplayEffectContext::SourceObject` 는 weak 포인터라 다음 GC 이후 null 이 된다. Instant GE 라면 같은 프레임에 끝나므로 무해하지만, Duration/Infinite GE 가 붙거나 주석(`:546`)이 말하는 "인스턴스 단위 데이터 추적" 을 실제로 하려는 소비처(ExecCalc·UI 등)가 생기면 조용히 null 을 본다. 충전형 경로는 인스턴스가 남으므로 영향 없다.
- **제안**: 소비형은 SourceObject 를 인스턴스 대신 `UWxItemDefinition`(정적 자산이라 수명 안전)으로 넘기거나, 인스턴스 추적이 꼭 필요하면 GE 적용을 차감보다 먼저 수행한다.
- **확신도**: 낮음 (현재 소비처가 Instant GE 뿐이라면 의도된 설계일 수 있음)

### 6. 🟢 데디케이티드 서버에서도 픽업 시각 에셋을 동기 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:135`, `:140`
- **범주**: 성능/안전
- **문제**: `ApplyPickupVisual` 은 `OnRep_ItemDef`(클라)뿐 아니라 서버 전용 진입점 `SetItemDef` 에서도 실행돼 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous` 한다. 물리 시뮬레이션 때문에 서버가 StaticMesh 를 필요로 하는 것은 맞지만 NiagaraSystem 은 순수 코스메틱이고, 엔진도 데디케이티드 서버에서는 `ActivateInternal` 초입의 `World->IsNetMode(NM_DedicatedServer)` 분기(`NiagaraComponent.cpp:1323`)로 재생 자체를 하지 않는다 — 로드분이 전부 낭비다. 드랍마다 불필요한 동기 로드 히치와 상주 메모리가 쌓인다.
- **제안**: Niagara 블록만 `IsNetMode(NM_DedicatedServer)` 로 건너뛴다(#2 와 같은 지점이라 함께 처리하면 된다). 지급·프롬프트 로직은 시각 에셋에 의존하지 않는다.
- **확신도**: 높음 (데디케이티드 서버를 운영할 때에 한해 영향)

### 7. 🟢 픽업 프롬프트가 키 리터럴 `[F]` 를 문자열에 박아 넣는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:113-114`
- **범주**: 설계/구조
- **문제**: `GetInteractionPrompt` 가 `"[F] {0} x{1}"` 형태로 키보드 키를 로컬라이즈 문자열에 포함한다. 키 리바인딩·게임패드에서 표기가 즉시 거짓이 되고, 번역가가 키까지 번역 대상으로 보게 된다. 같은 `IWxInteractable` 를 구현한 다른 곳은 전부 키를 붙이지 않는다 — `UWxDialogueComponent::GetInteractionPrompt` 는 `"Talk to {0}"`, `AWxEnemyCharacter` 는 `"Finisher"`, 기믹은 데이터 자산의 `Prompt` 를 그대로 돌려준다. 즉 이 모듈만 규약에서 벗어나 있고, HUD 가 나중에 키 표시를 붙이면 이중 표기가 된다.
- **제안**: 문자열에서 `[F]` 를 빼고 아이템 이름/수량만 돌려준다. 키 표기가 필요하면 프롬프트를 소비하는 HUD 쪽(`UWxInteractionScannerComponent` 소비처)이 현재 바인딩으로 조립한다.
- **확신도**: 높음

### 8. 🟢 uplugin 이 Niagara 의존을 선언하지 않는다
- **위치**: `Plugins/WxInventory/WxInventory.uplugin:16-33`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs:26`
- **범주**: 설계/구조
- **문제**: Build.cs 가 `Niagara` 를 PrivateDependency 로 쓰고 `AWxItemPickup` 이 `UNiagaraComponent`/`UNiagaraSystem` 을 직접 참조하는데, uplugin 의 `Plugins` 배열에는 `WxCore`·`GameplayAbilities`·`ModularGameplay`·`StateTree` 만 있고 Niagara 가 빠져 있다. 지금은 Niagara 가 엔진 기본 활성이라 통하지만, 나머지 세 엔진 플러그인은 전부 선언한 것과 어긋나고 비활성 환경에서는 플러그인 의존성 오류 대신 컴파일 오류로 드러난다.
- **제안**: uplugin 의 `Plugins` 배열에 `Niagara` 를 추가한다.
- **확신도**: 높음

### 9. 🟢 `GrantReward` 가 `UWorld` 를 널 검증 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:38` (획득), `:70` (`World->SpawnActorDeferred`)
- **범주**: 버그/정확성
- **문제**: 임의의 `AActor*` 를 받는 public static `BlueprintCallable` 진입점인데 `SourceActor->GetWorld()` 결과를 검사하지 않는다. `HasAuthority()` 는 `ROLE_Authority` 검사라 월드에 속하지 않은 객체도 통과할 수 있고, 그 경우 픽업 보상 항목에서 널 역참조로 크래시한다. 모듈의 다른 널 경로는 전부 가드돼 있어 이 한 곳만 비어 있다.
- **제안**: 한 줄 조기 반환을 넣거나, 저ROI 로 판단해 넣지 않을 거면 그 판단을 주석으로 남긴다 — 여러 리뷰에 걸쳐 반복 지적되고 있으니 이번에 어느 쪽이든 결론을 코드에 남기는 편이 낫다.
- **확신도**: 낮음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, 경계 확인용 외부 호출부(`Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Plugins/WxDialogue/.../WxDialogueComponent.cpp`, `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp`)
- **미검토 / 한계**: 런타임 검증(PIE 2인 이상 네트워크 테스트)은 하지 않았다. #1·#2 는 엔진 소스 기반 정적 판단이라 실제 재현이 남아 있고, #2 의 증상 크기를 좌우하는 픽업 BP 의 Niagara 기본 에셋은 이번 패스에서 확인하지 못했다(`Plugins/WxBlueprintSnapshot/Snapshots/` 가 비어 있고 이전 리뷰가 쓰던 `.claude/asset_dump/` 는 사라졌다 — BP 근거가 필요하면 스냅샷을 먼저 갱신할 것). BP/WBP 내부와 ItemDefinition 데이터 자산의 Fragment 조합 타당성은 범위 밖이다.

  반대로 이번에 확인해 **문제없다고 판단한 것**은 다음 세션이 중복 조사하지 않도록 남긴다 — (a) **서브오브젝트 복제 정상(과거 🔴 오탐, 재조사 금지)**: 컴포넌트만 `bReplicateUsingRegisteredSubObjectList`(`WxInventoryManagerComponent.cpp:225`)를 켜고 소유 `APlayerController` 는 끈 조합을 엔진이 정확히 지원한다(레거시 `AActor::ReplicateSubobjects` → `UActorChannel::ReplicateSubobject` 의 `checkf(Actor->IsUsingRegisteredSubObjectList() == false)` 가 이 분기의 전제를 못박는다). (b) FastArray 참조 해결 순서 안전: `PostReplicatedAdd` 시점에 `Entry.Instance` 는 유효하며, 코드도 `if (Entry.Instance)` 로 한 번 더 가린다. (c) **픽업 물리 복제 정상**: 클라는 `LaunchInDirection` 이 조기 반환하지만 `AActor::PostNetReceive` → `SyncReplicatedPhysicsSimulation`(`ActorReplication.cpp`)이 `bRepPhysics` 에 맞춰 클라 루트의 `SetSimulatePhysics` 를 대신 켜준다 — `SetReplicateMovement(true)`(`WxItemPickup.cpp:24`)만으로 충분하다. (d) **모듈 경계**: Build.cs/uplugin 의 Wx 의존은 `WxCore` 뿐, 소스 내 비-WxInventory Wx 인클루드도 `WxInteractable.h`·`WxGameplayTags.h`·`WxCollisionChannels.h`(전부 WxCore) 뿐이다. (e) **규칙 위반 0건**(기계 검사): 22개 소스 전부 첫 줄 저작권 표기 존재, 람다·`FORCEINLINE`/`inline` 0건, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 한 곳뿐, Wx prefix 누락·`Super::` 누락 없음, 델리게이트 바인딩이 모듈 내에 없어 `Handle` 규칙 대상 없음. 헤더 인라인은 템플릿 `FindFragmentByClass<T>()` 와 StateTree 관용 `GetInstanceDataType()` 뿐이며 둘 다 예외 사유가 주석에 있다. (f) 통지 수렴: 클라 `PreReplicatedRemove` 가 제거 대상 `StackCount` 를 0 으로 내린 뒤 총량을 재계산하므로 서버/클라 최종 합계가 일치한다(배치 제거 중 중간 방송값은 과대일 수 있으나 최종값은 수렴). (g) 권위: `AddItemDefinition`/`ConsumeItemsByDefinition`/`UseItemByDef`/`RefillItemCharges` 의 `check(HasAuthority())` 는 모든 호출 경로(서버 전용 `OnInteracted`, `WxAbility_UseItem::HandleConsumeEvent` 의 `HasAuthority` 게이트, 권위 게이트된 StateTree Task)에서 안전하다. (h) 초기 상태 유실 없음: `UWxViewModel_Inventory::Initialize` 가 구독 직후 `RefreshAllItems()` 로 pull 하므로 `OnAnyInventoryReady` 이전에 도착한 FastArray 추가분을 놓치지 않는다.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 22파일 — `/module-review`로 갱신*
