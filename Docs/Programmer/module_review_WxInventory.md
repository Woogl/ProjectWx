# WxInventory — 코드 리뷰

> 건강한 모듈이다. FastArray + 등록 서브오브젝트 복제, 권위 게이팅, 서버/클라 통지 경로 수렴이 일관되게 설계돼 있고 CLAUDE.md 규칙 위반도 거의 없으며, 지난 리뷰의 🔴(클라 합계 과다 계산)도 해결됐다. 남은 문제는 "로컬 플레이어 1명" 전제가 도메인 API 에 스며든 것과, 장비 경로가 아직 호출부 없는 미완 상태라는 두 축이다. 이번 리뷰는 `Plugins/WxInventory` 소스 21개를 전부 읽고 매니저·장비·픽업·보상 cpp 는 라인 단위로 검토했으며, 외부 호출부(WxGame/WxWorld)와 엔진 복제 경로(UE 5.8 `DataChannel.cpp`)까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 보상 지급·충전 리필 대상이 0번 PlayerController 로 하드코딩돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:42`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp:35`
- **범주**: 설계/구조
- **문제**: 두 StateTree Task 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 정한다. 주석은 "로컬 플레이어(0번 컨트롤러)" 라고 적었지만, 데디케이티드 서버에는 로컬 플레이어가 없고 `GetPlayerController(World, 0)` 은 **먼저 접속한 클라이언트**를 돌려준다. 즉 플레이어 2가 보물상자를 열면 재화가 플레이어 1의 인벤토리로 들어가고, 플레이어 2가 체크포인트를 밟으면 플레이어 1의 에스트병이 채워진다(본인은 못 받음). 모듈의 나머지(FastArray 복제, 서브오브젝트 복제, 전 진입점 `HasAuthority` 게이팅)는 멀티플레이를 전제로 꼼꼼히 짜여 있어 이 부분만 전제가 어긋난다. 발동 주체는 `IWxInteractable::OnInteracted(Interactor, Source)` 로 이미 권위 측에 도달해 있으므로(`Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:56`) 정보 자체는 존재한다. 같은 패턴이 `Source/WxGame/Character/WxEnemyCharacter.cpp:69` 에도 있다.
- **제안**: 두 Task 의 InstanceData 에 대상 Actor 바인딩 핀을 추가해 StateTree 에서 기믹이 들고 있는 상호작용 instigator(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:56`)를 꽂도록 바꾼다. 바인딩이 비어 있을 때만 현재의 0번 PC 폴백을 유지한다.
- **확신도**: 중간 (싱글플레이 범위를 전제한 의도적 단순화일 수 있음 — 주석이 그 전제를 두 곳에서 명시하고 있다)

### 2. 🟡 장비 경로 전체가 호출부 없는 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:579` (`EquipItemByDef`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34` (`EquipItem`)
- **범주**: 중복/복잡도
- **문제**: 저장소 전체(`Plugins/`, `Source/`, BP 스냅샷 JSON 포함)를 검색해도 `EquipItemByDef` 호출부가 0건이고, `UWxEquipmentComponent::EquipItem` 은 그 `EquipItemByDef` 에서만 호출된다. 둘 다 `BlueprintCallable` 이 아니라 BP 에서도 호출할 수 없다. 결과적으로 `EquippedItemDef` 는 영원히 null 이고, `AWxCharacterBase::PostInitializeComponents` 가 구독해 둔 `OnEquipVisualChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:73`)는 아이템과 함께 fire 되는 일이 없으며, `UWxItemFragment_Equippable::EquipEffects` 도 적용되지 않는다. 즉 "장비 착용" 은 배선만 있고 트리거가 없다. `RemoveItemInstance`(`WxInventoryManagerComponent.cpp:347`)도 마찬가지로 저장소 전체에 호출부가 0건이다.
- **제안**: 장비 착용을 실제로 쓸 계획이면 트리거(UI 슬롯 → 어빌리티/서버 RPC → `EquipItemByDef`)를 붙여 경로를 닫고, 당분간 안 쓸 계획이면 미구현임을 README·헤더 주석에 명시해 다음 세션이 "이미 동작한다"고 오해하지 않게 한다.
- **확신도**: 높음

### 3. 🟡 `UWxEquipmentComponent` 의 현재 장착 상태를 pull 할 수단이 없어 늦은 구독자는 외형을 놓친다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:48` (델리게이트), `:58-59` (protected `EquippedItemDef`, public getter 없음)
- **범주**: 설계/구조
- **문제**: 외형 반영이 `OnEquipVisualChanged` 방송 **한 번**에만 의존한다. 네트워크 스폰 액터는 초기 복제 프로퍼티 적용(및 그때의 RepNotify)이 `PostInitializeComponents`/`BeginPlay` 보다 먼저 일어나므로, 이미 장비를 착용한 폰이 뒤늦게 relevant 해진 클라이언트에서는 `OnRep_EquippedItemDef` → `BroadcastEquipVisual` 이 구독자가 붙기 전에 실행돼 방송이 유실되고 기본 메시로 남는다. 구독 시점에 현재 상태를 다시 물어볼 API 가 없어 복구도 불가능하다. 같은 프로젝트의 `AWxCharacterBase` 는 래그돌 태그에 대해 "late join 시 구독보다 먼저 초기 복제로 태그가 실려 왔을 수 있어 1회 즉시 확인한다" 는 보정을 이미 하고 있는데(`Source/WxGame/Character/WxCharacterBase.cpp:65-69`), 장비에는 그 대응물이 없다.
- **제안**: `GetEquippedItemDef()` public getter 또는 `RefreshEquipVisual()`(현재 상태로 1회 재방송)을 노출해 구독 직후 게임 측이 pull 할 수 있게 한다. #2 를 해결해 장비가 실제로 착용되기 시작하면 바로 드러날 문제다.
- **확신도**: 중간

### 4. 🟢 override 에서 `Super::` 를 호출하지 않는 곳이 있다 (코딩 규칙 5)
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp:18` (`IsSupportedForNetworking`), `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12` (`GetPrimaryAssetId`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp:18` 및 `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:18` (`EnterState`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:49` (`GetDescription`)
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5는 예외 없이 override 시 `Super::` 호출을 요구한다. 모듈 내 다른 override(`GetLifetimeReplicatedProps`, `BeginPlay`, `EndPlay`, `ReadyForReplication`, `OnInstanceCreated`, `PostEditChangeProperty`)는 모두 지키고 있어 위 5곳만 예외로 남는다. 지난 리뷰에서 3곳이 지적됐고 그 뒤 StateTree 노드가 늘면서 2곳이 추가됐다.
- **제안**: 전부 부모 반환값을 대체하는 값 반환 override 라 기계적 `Super::` 호출은 오히려 오동작을 부른다(`FStateTreeTaskCommonBase::EnterState` 는 `Running` 을 반환). 코드를 바꾸기보다 규칙 5에 "값 반환 대체 override 는 예외" 를 명문화하거나, 각 지점에 예외 근거를 한 줄 주석으로 남겨 반복 지적을 끊는 편이 낫다.
- **확신도**: 낮음 (UE 관용상 정당하며 의도된 설계로 보임 — 규칙 문서 쪽 정리가 실질 해법)

### 5. 🟢 데디케이티드 서버에서도 픽업 시각 에셋을 동기 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:53` (`SetItemDef` → `ApplyPickupVisual`), 실제 로드는 `:132`, `:137`
- **범주**: 성능/안전
- **문제**: `ApplyPickupVisual` 은 `OnRep_ItemDef`(클라) 뿐 아니라 `SetItemDef`(서버 전용 진입점, `UWxRewardLibrary::GrantReward` 에서 호출)에서도 실행돼 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous` 하고 `NiagaraComponent->Activate(true)` 까지 한다. 데디케이티드 서버는 이 에셋들이 필요 없으므로 드랍마다 불필요한 동기 로드 히치와 상주 메모리가 발생한다. 오픈월드에서 드랍 종류가 늘수록 누적된다.
- **제안**: `ApplyPickupVisual` 진입부에서 `IsNetMode(NM_DedicatedServer)` 이면 조기 반환한다. 상호작용 프롬프트·지급 로직은 시각 에셋에 의존하지 않으므로 서버 동작에 영향이 없다.
- **확신도**: 높음 (데디케이티드 서버를 운영할 때에 한해 영향)

### 6. 🟢 소비되는 아이템 인스턴스를 GE SourceObject 로 넘긴 뒤 그 인스턴스를 파괴한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:525` (`Context.AddSourceObject(SourceInstance)`), `:543` (`ConsumeItemsByDefinition`), `:550` (`ApplyGameplayEffectSpecToSelf`)
- **범주**: 버그/정확성
- **문제**: 비충전형(Charges Fragment 없음) 경로에서는 `ConsumeItemsByDefinition` 이 마지막 1개를 차감하면서 엔트리를 제거하고 `UWxItemInstance` 를 고아로 만든다. 그 직후 적용되는 GE 의 `SourceObject` 는 그 인스턴스를 가리키는데, `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 이라 다음 GC 이후 null 이 된다. Instant GE 라면 같은 프레임에 끝나므로 무해하지만, Duration/Infinite GE 이거나 `AddSourceObject` 의 주석대로 "인스턴스 단위 데이터 추적" 을 나중에 하려는 소비처(ExecutionCalculation·UI 등)가 생기면 조용히 null 을 보게 된다. 충전형 경로는 인스턴스가 남으므로 영향이 없다.
- **제안**: 소비형 아이템은 SourceObject 를 인스턴스 대신 `UWxItemDefinition`(정적 자산이라 수명 안전)으로 넘기거나, 인스턴스 추적이 꼭 필요하면 GE 적용을 차감보다 먼저 수행한다.
- **확신도**: 낮음 (현재 소비처가 Instant GE 뿐이라면 의도된 설계일 수 있음)

### 7. 🟢 `GrantReward` 가 `UWorld` 를 널 검증 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:39` (획득), `:76` (`World->SpawnActorDeferred`)
- **범주**: 버그/정확성
- **문제**: 임의의 `AActor*` 를 받는 public static 진입점인데 `SourceActor->GetWorld()` 결과를 검사하지 않는다. `HasAuthority()` 는 `ROLE_Authority` 검사라 CDO/기본 객체나 파괴 진행 중인 액터도 통과할 수 있고, 그 경우 픽업 보상 항목에서 널 역참조로 크래시한다. 모듈의 다른 모든 널 경로는 꼼꼼히 가드돼 있어 이 한 곳만 비어 있다. 지난 리뷰에서도 지적됐고 아직 그대로다.
- **제안**: `World` 획득 직후 널이면 조기 반환한다(한 줄).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp`
- **훑은 파일**: `Public/Items/WxItemFragment.h`, `Private/Items/WxItemFragment.cpp`, `Public/Items/WxItemDefinition.h`, `Private/Items/WxItemDefinition.cpp`, `Public/Items/WxRewardTableRow.h`, `Public/Items/WxItemPickup.h`, `Public/Items/WxItemInstance.h`, `Public/Inventory/WxEquipmentComponent.h`, `Public/Inventory/WxRewardStateTreeNodes.h`, `Public/Inventory/WxInventoryStateTreeNodes.h`, `Public/WxRewardLibrary.h`, `WxInventory.Build.cs`, `Public/WxInventoryModule.h`, `Private/WxInventoryModule.cpp` (모두 `Plugins/WxInventory/Source/WxInventory/` 기준)

- **확인해서 문제 없다고 판단한 항목** (다음 세션이 중복 조사하지 않도록 기록):
  - 모듈 경계: `WxInventory.Build.cs` 의 Wx 의존은 `WxCore` 뿐이고, 소스 내 비-WxInventory Wx 인클루드도 `WxCollisionChannels.h`/`WxInteractable.h`/`WxGameplayTags.h`(전부 WxCore) 뿐이다.
  - 규칙 준수: 전 파일 첫 줄 저작권 표기 존재, 람다 0건, `BlueprintCallable` 은 `UWxRewardLibrary`(BP Function Library) 한 곳뿐, Wx prefix 누락 없음, 델리게이트 콜백 `Handle`/`OnRep_`/`Notify*` 명명 일관.
  - FastArray 정합성: `AddEntry`/`RemoveEntry`/`AddToEntryStack`/`ConsumeByDefinition` 의 `MarkItemDirty`/`MarkArrayDirty` 조합과 `CreateIterator`+`RemoveCurrent` 순회는 모두 올바르다.
  - 서브오브젝트 복제 순서: 소유 액터(PlayerController)가 `bReplicateUsingRegisteredSubObjectList` 를 켜지 않고 컴포넌트만 켠 혼합 구성이지만, UE 5.8 은 이를 정식 지원한다(`DataChannel.cpp:4338-4389`, `AActor::ReplicateSubobjects` 경유). 두 경로 모두 서브오브젝트를 컴포넌트보다 먼저 번치에 쓰므로(`DataChannel.cpp:4227` 의 "SubObjects have to be created before the component on the receiving end") `PostReplicatedAdd` 시점에 `Entry.Instance` 와 그 `ItemDef` 는 이미 해석돼 있다 — 지난 리뷰 #2(언맵 서브오브젝트로 인한 추가 통지 유실)는 엔진이 순서를 보장하므로 이슈가 아니다.
  - `AWxItemPickup::OnInteracted` 의 "서버 권위에서만 호출" 전제: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:99` 가 유일한 호출부이며 서버 권위 어빌리티다.

- **지난 리뷰(`9661edf`, 2026-07-21) 대비**:
  - 해결됨 — 이전 🔴(클라 `PreReplicatedRemove` 의 정의 합계 과다 계산)은 `WxInventoryManagerComponent.cpp:69-79` 에서 제거 대상 엔트리의 `StackCount` 를 재계산 전에 0으로 내리는 방식으로 고쳐졌다. 이전 🟢 #4(`DeveloperSettings` 미사용 의존성)도 `Build.cs` 에서 제거됐다.
  - 남아 있으나 이번 목록에서 제외 — 이전 #3(머지 루프 재진입)은 현 코드(`:296-320`)에 그대로 있으나, 잡아 두는 것은 `TArray` 자체 참조라 realloc 으로 무효화되지 않고 인덱스 시프트 위험만 남으며(이전 리뷰의 "해제 후 접근" 기술은 과장) 현 구독자가 읽기 전용 MVVM 뿐이라 발현 조건이 없다. 이전 #6(Charges + Stackable MaxStack>1 동시 부착)도 그대로이나 기획자 오설정 전제라 문서 가드로 충분하다는 이전 판단을 유지한다.

- **미검토 / 한계**: 런타임 검증(PIE 2인 이상 네트워크 테스트)은 하지 않았다. 특히 #1(0번 PC)과 #3(늦은 구독자 외형 유실)은 정적 근거만으로 판단한 것이라 실제 재현 확인이 남아 있다. BP/WBP 내부(픽업 상속 BP 의 Niagara 에셋 지정, ItemDefinition 데이터 자산의 실제 Fragment 조합, 인벤토리 WBP 바인딩)는 범위 밖으로 두었다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 21파일 — `/module-review`로 갱신*
