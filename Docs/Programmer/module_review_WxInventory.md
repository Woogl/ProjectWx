# WxInventory — 코드 리뷰

> 건강한 모듈이다. FastArray + 등록 서브오브젝트 복제, 전 진입점 권위 게이팅, 서버 변경 경로와 클라 복제 콜백이 같은 `Notify*` 진입점으로 수렴하는 구조가 일관되고, CLAUDE.md 규칙 위반도 사실상 없다. 남은 문제는 "로컬 플레이어 1명" 전제가 도메인 API 에 스며든 것과 장비 경로가 호출부 없는 미완 상태라는 두 축이며, 그 외는 방어 코드 수준의 사소한 항목이다. 이번 리뷰는 소스 21개를 모두 열고 `WxInventoryManagerComponent.cpp`·`WxEquipmentComponent.cpp`·`WxItemPickup.cpp`·`WxRewardLibrary.cpp` 는 라인 단위로, 외부 호출부(`Source/WxGame` 의 MVVM·어빌리티·GameMode)는 경계 확인 목적으로 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 보상 지급·충전 리필 대상이 0번 PlayerController 로 하드코딩돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp:34`
- **범주**: 설계/구조
- **문제**: 두 StateTree Task 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 정한다. 주석은 "로컬 플레이어(0번 컨트롤러)" 라고 적었지만 데디케이티드 서버에는 로컬 플레이어가 없고, 이 호출은 **먼저 접속한 클라이언트**를 돌려준다. 플레이어 2가 상자를 열면 재화가 플레이어 1에게 들어가고, 플레이어 2가 체크포인트를 밟으면 플레이어 1의 에스트병이 채워진다. 모듈의 나머지(FastArray 복제, 서브오브젝트 복제, 전 진입점 `HasAuthority` 게이팅)는 멀티플레이를 전제로 촘촘히 짜여 있어 이 두 지점만 전제가 어긋난다. 발동 주체 정보는 `IWxInteractable::OnInteracted(Interactor, Source)` 로 이미 권위 측에 도달해 있으므로 존재하지 않는 정보가 아니다.
- **제안**: 두 Task 의 InstanceData 에 대상 Actor 바인딩 핀을 추가해 StateTree 에서 기믹이 들고 있는 상호작용 instigator 를 꽂도록 바꾸고, 바인딩이 비었을 때만 현재의 0번 PC 폴백을 유지한다.
- **확신도**: 중간 (싱글플레이 범위를 전제한 의도적 단순화일 수 있음 — 주석이 그 전제를 두 곳에서 명시하고 있다)

### 2. 🟡 장비 경로 전체가 호출부 없는 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:609` (`EquipItemByDef`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34` (`EquipItem`)
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 `EquipItem` 문자열이 나오는 곳은 WxInventory 자신과 `Docs/` 뿐이다. 즉 `EquipItemByDef` 의 호출부가 0건이고, `UWxEquipmentComponent::EquipItem` 은 그 `EquipItemByDef`(`:635`) 에서만 호출된다. 둘 다 `BlueprintCallable` 이 아니라 BP 에서도 부를 수 없다. 결과적으로 `EquippedItemDef` 는 영원히 null 이고, `AWxCharacterBase::PostInitializeComponents` 가 구독해 둔 `OnEquipVisualChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:82`) 는 아이템과 함께 fire 되지 않으며 `UWxItemFragment_Equippable::EquipEffects` 도 적용되지 않는다. 배선만 있고 트리거가 없는 상태다. `RemoveItemInstance`(`WxInventoryManagerComponent.cpp:364`) 도 마찬가지로 호출부가 0건이다.
- **제안**: 장비를 실제로 쓸 계획이면 트리거(UI 슬롯 → 어빌리티/서버 RPC → `EquipItemByDef`)를 붙여 경로를 닫는다. 당분간 안 쓸 계획이면 미구현임을 README 와 헤더 주석에 명시해 다음 세션이 "이미 동작한다"고 오해하지 않게 한다.
- **확신도**: 높음

### 3. 🟡 `UWxEquipmentComponent` 의 현재 장착 상태를 pull 할 수단이 없어 늦은 구독자는 외형을 놓친다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:48` (델리게이트), `:58-59` (protected `EquippedItemDef`, public getter 없음)
- **범주**: 설계/구조
- **문제**: 외형 반영이 `OnEquipVisualChanged` 방송 **한 번**에만 의존한다. 네트워크 스폰 액터는 초기 복제 프로퍼티 적용(및 그때의 RepNotify)이 게임 측 구독 시점보다 먼저 일어날 수 있으므로, 이미 장비를 착용한 폰이 뒤늦게 relevant 해진 클라이언트에서는 `OnRep_EquippedItemDef` → `BroadcastEquipVisual` 이 구독자가 붙기 전에 실행돼 방송이 유실되고 기본 메시로 남는다. 구독 시점에 현재 상태를 다시 물을 API 가 없어 복구도 불가능하다. 같은 프로젝트의 `AWxCharacterBase` 는 래그돌 태그에 대해 "구독보다 먼저 초기 복제가 왔을 수 있으니 1회 즉시 확인" 보정을 이미 하고 있는데 장비에는 그 대응물이 없다. #2 때문에 아직 발현하지 않을 뿐 트리거를 붙이는 순간 드러난다.
- **제안**: `GetEquippedItemDef()` public getter 또는 `RefreshEquipVisual()`(현재 상태로 1회 재방송)을 노출해 구독 직후 게임 측이 pull 할 수 있게 한다.
- **확신도**: 중간

### 4. 🟢 `GrantReward` 가 `UWorld` 를 널 검증 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:39` (획득), `:76` (`World->SpawnActorDeferred`)
- **범주**: 버그/정확성
- **문제**: 임의의 `AActor*` 를 받는 public static `BlueprintCallable` 진입점인데 `SourceActor->GetWorld()` 결과를 검사하지 않는다. `HasAuthority()` 는 `ROLE_Authority` 검사라 월드에 속하지 않은 객체나 파괴 진행 중인 액터도 통과할 수 있고, 그 경우 픽업 보상 항목에서 널 역참조로 크래시한다. 모듈의 다른 널 경로는 전부 가드돼 있어 이 한 곳만 비어 있다. 지난 리뷰에서도 지적됐고 그대로다.
- **제안**: `World` 획득 직후 널이면 조기 반환한다(한 줄).
- **확신도**: 중간

### 5. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:288`, `:324-342`
- **범주**: 성능/안전
- **문제**: `const int32 MaxStack = Stackable ? Stackable->MaxStack : 1;` 로 값을 받은 뒤 `while (Remaining > 0) { const int32 ChunkCount = FMath::Min(MaxStack, Remaining); ... Remaining -= ChunkCount; }` 로 잔여분을 나눈다. `MaxStack` 이 0 이하이면 `ChunkCount` 가 0 이하가 되어 `Remaining` 이 줄지 않고, 매 회 `AddEntry` 로 엔트리를 만들며 무한 루프 → 프로세스 행/OOM 이다. `UWxItemFragment_Stackable::MaxStack` 의 `meta = (ClampMin = "1")`(`Public/Items/WxItemFragment.h:144`)은 디테일 패널 입력만 막을 뿐 직렬화된 값이나 프로퍼티 붙여넣기까지 강제하지 않는다.
- **제안**: `MaxStack` 을 읽는 지점에서 `FMath::Max(1, ...)` 로 하한을 강제한다(한 줄). 루프 자체를 방어할 필요는 없다.
- **확신도**: 중간 (정상 편집 경로는 ClampMin 이 막고 있어 데이터 신뢰를 전제한 설계일 수 있음)

### 6. 🟢 데디케이티드 서버에서도 픽업 시각 에셋을 동기 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:53` (`SetItemDef` → `ApplyPickupVisual`), 실제 로드는 `:132`, `:137`
- **범주**: 성능/안전
- **문제**: `ApplyPickupVisual` 은 `OnRep_ItemDef`(클라) 뿐 아니라 서버 전용 진입점인 `SetItemDef`(`UWxRewardLibrary::GrantReward` 가 호출)에서도 실행돼 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous` 하고 `NiagaraComponent->Activate(true)` 까지 한다. 데디케이티드 서버는 이 에셋이 필요 없으므로 드랍마다 불필요한 동기 로드 히치와 상주 메모리가 발생하고, 오픈월드에서 드랍 종류가 늘수록 누적된다.
- **제안**: `ApplyPickupVisual` 진입부에서 `IsNetMode(NM_DedicatedServer)` 이면 조기 반환한다. 상호작용 프롬프트·지급 로직은 시각 에셋에 의존하지 않으므로 서버 동작에는 영향이 없다.
- **확신도**: 높음 (데디케이티드 서버를 운영할 때에 한해 영향)

### 7. 🟢 소비되는 아이템 인스턴스를 GE SourceObject 로 넘긴 뒤 그 인스턴스를 파괴한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:555` (`Context.AddSourceObject(SourceInstance)`), `:573` (`ConsumeItemsByDefinition`), `:580` (`ApplyGameplayEffectSpecToSelf`)
- **범주**: 버그/정확성
- **문제**: 비충전형(Charges Fragment 없음) 경로에서는 `ConsumeItemsByDefinition` 이 마지막 1개를 차감하며 엔트리를 제거해 `UWxItemInstance` 를 고아로 만든다. 그 직후 적용되는 GE 의 `SourceObject` 는 그 인스턴스를 가리키는데 `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 이라 다음 GC 이후 null 이 된다. Instant GE 라면 같은 프레임에 끝나므로 무해하지만, Duration/Infinite GE 가 붙거나 주석이 말하는 "인스턴스 단위 데이터 추적" 을 나중에 하려는 소비처(ExecutionCalculation·UI 등)가 생기면 조용히 null 을 보게 된다. 충전형 경로는 인스턴스가 남으므로 영향 없다.
- **제안**: 소비형은 SourceObject 를 인스턴스 대신 `UWxItemDefinition`(정적 자산이라 수명 안전)으로 넘기거나, 인스턴스 추적이 꼭 필요하면 GE 적용을 차감보다 먼저 수행한다.
- **확신도**: 낮음 (현재 소비처가 Instant GE 뿐이라면 의도된 설계일 수 있음)

### 8. 🟢 `WxRewardTableRow.h` 가 헤더에 인라인 함수를 정의한다 (코딩 규칙 6)
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h:32-35` (`FWxItemRewardEntry::IsValid`), `:73-81` (`FWxRewardTableRow::GetValidRewards`)
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 6은 인라인 함수 정의를 금지한다. 이 두 함수는 템플릿이 아니고 대응 `.cpp` 도 없이 클래스 본문에서 바로 정의돼 있다. 모듈 내 다른 헤더 인라인은 `FindFragmentByClass<T>()`(템플릿이라 헤더 정의 불가피)와 StateTree 의 `GetInstanceDataType()`(엔진 관용이며 프로젝트 전 플러그인 9개 파일에서 동일) 뿐이라, 규칙에서 벗어난 것은 이 파일뿐이다.
- **제안**: `WxRewardTableRow.cpp` 를 추가해 두 함수 본문을 옮긴다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, 경계 확인용 외부 호출부(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`)
- **미검토 / 한계**: 런타임 검증(PIE 2인 이상 네트워크 테스트)은 하지 않았다. #1(0번 PC)과 #3(늦은 구독자 외형 유실)은 정적 근거만으로 판단한 것이라 실제 재현 확인이 남아 있다. BP/WBP 내부(픽업 상속 BP 의 Niagara 지정, ItemDefinition 데이터 자산의 실제 Fragment 조합, 인벤토리 WBP 바인딩)는 범위 밖이다. 반대로 이번에 확인해 **문제없다고 판단한 것**은 다음 세션이 중복 조사하지 않도록 기록해 둔다 — (a) 모듈 경계: `Build.cs`/`uplugin` 의 Wx 의존은 `WxCore` 뿐이고 소스 내 비-WxInventory Wx 인클루드도 `WxInteractable.h`/`WxGameplayTags.h`/`WxCollisionChannels.h`(전부 WxCore) 뿐이다. (b) 규칙 준수: 21개 소스 전부 첫 줄 저작권 표기 존재, 람다 0건, `BlueprintCallable` 은 `UWxRewardLibrary`(BP Function Library) 한 곳뿐, Wx prefix 누락 없음, 델리게이트 바인딩 자체가 모듈 내에 없어 `Handle` 규칙 대상 없음. (c) FastArray 정합성: `AddEntry`/`RemoveEntry`/`AddToEntryStack`/`ConsumeByDefinition` 의 `MarkItemDirty`/`MarkArrayDirty` 조합과 `CreateIterator`+`RemoveCurrent` 순회, `LastObservedCount` 의 `NotReplicated` 지정(엔진 `FFastArraySerializerItem` 과 동일 관용) 모두 올바르다. (d) 통지 수렴: 클라 `PreReplicatedRemove` 가 제거 대상 `StackCount` 를 0으로 내린 뒤 총량을 재계산하므로 서버/클라 최종 합계가 일치한다. (e) `OnAnyInventoryReady` 가 클라에서 인벤토리 도착보다 먼저 발행되더라도, 구독자(`UWxViewModel_Inventory`)가 `OnInventoryStackChanged` 로 후속 추가를 받아 따라잡으므로 초기 상태 유실이 없다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 21파일 — `/module-review`로 갱신*
