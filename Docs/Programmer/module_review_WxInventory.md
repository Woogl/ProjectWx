# WxInventory — 코드 리뷰

> 2,291줄 22파일의 작은 도메인 플러그인으로, 규칙 준수(저작권 헤더·`Wx` prefix·람다 부재·`BlueprintCallable` 한정·`WxCore` 외 의존 없음)와 FastArray/서브오브젝트 복제 설계는 정확하며, 서버 직접 발행과 클라 복제 콜백이 같은 통지 진입점으로 수렴하는 구조도 검증상 문제가 없다. 남은 결함은 보상 드랍 파이프라인(동시 스폰·동기 로드)과 아직 트리거가 없는 장비 경로에 몰려 있다. 이번 리뷰는 `.uplugin`/`*.Build.cs`와 헤더 전량, `WxInventoryManagerComponent.cpp`·`WxRewardLibrary.cpp`·`WxItemPickup.cpp`·`WxEquipmentComponent.cpp`·`WxItemInstance.cpp` 등 핵심 cpp 전량을 읽었고, FastArray 수신 경로·서브오브젝트 복제 경로는 UE 5.8 엔진 소스로, 외부 호출부는 저장소 전역 grep 으로 대조 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 한 보상 Row 의 픽업들이 완전히 같은 위치·같은 속도로 스폰된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:40-79`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:31-37`
- **범주**: 버그/정확성
- **문제**: 보상 루프가 항목마다 동일한 `SpawnTransform` 으로 스폰하고(`:70`) 동일한 `LaunchVelocity` 로 발사한다(`:79`). 픽업 루트 메시는 `ECC_WorldDynamic` + `SetCollisionResponseToAllChannels(ECR_Block)` 이라 픽업끼리 서로 Block 하고, `LaunchInDirection` 이 `SetSimulatePhysics(true)` 를 건다. `FWxRewardTableRow` 는 픽업 보상을 최대 5개까지 담으므로(`Public/Items/WxRewardTableRow.h:37-50`), 적 하나가 픽업 2개 이상을 떨구면 완전히 겹친 채 스폰돼 디페네트레이션 임펄스로 튀어 나간다. 유일한 실호출부가 적 처치(`Source/WxGame/Character/WxEnemyCharacter.cpp:60`)이고 그쪽은 `FVector::UpVector * LaunchSpeed` 라 수평 산포가 아예 없어, 다중 드랍이면 매번 재현된다.
- **제안**: 루프 인덱스로 스폰 위치를 부채꼴 분산시키거나 `LaunchVelocity` 에 항목별 수평 산포를 더한다. 픽업 전용 오브젝트 채널을 주어 픽업끼리 Ignore 하게 하는 방법도 있다.
- **확신도**: 중간

### 2. 🟡 보상 지급 파이프라인이 적 처치 핫패스에서 게임스레드 동기 로드를 4연발한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:42`, `:62`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:135`, `:140`
- **범주**: 성능/안전
- **문제**: 보상 항목마다 `Reward.Item.LoadSynchronous()`(ItemDefinition) → `PickupFragment->ItemActorClass.LoadSynchronous()`(픽업 BP 클래스, 의존 에셋 전부 동반) → `ApplyPickupVisual` 의 `Mesh.LoadSynchronous()` + `NiagaraSystem.LoadSynchronous()` 가 연달아 게임스레드에서 돈다. 주석은 "산발적 호출이라 무방"으로 정책화했지만(`WxInventoryManagerComponent.cpp:349`) 실제 호출부는 적 처치라 산발적이지 않고, 첫 등장 아이템마다 전투 종료 프레임에 히치가 난다. 클라이언트도 `OnRep_ItemDef` → `ApplyPickupVisual` 로 패킷 처리 중에 같은 동기 로드를 수행한다. 데디케이티드 서버에서는 표시에만 쓰이는 Niagara 시스템까지 로드한다.
- **제안**: 픽업 BP 클래스·메시는 Experience/GameFeature 번들이나 AssetManager 번들로 선로드해 `LoadSynchronous` 가 캐시 히트가 되게 한다. Niagara 로드는 `IsNetMode(NM_DedicatedServer)` 로 가르고, 최소한 픽업 비주얼은 비동기 로드 후 적용으로 바꾼다.
- **확신도**: 중간

### 3. 🟡 `UseItemByDef` 가 소비로 사라질 인스턴스를 GE SourceObject 로 물린다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:525-569`
- **범주**: 버그/정확성
- **문제**: `Context.AddSourceObject(SourceInstance)`(`:544`) 로 Spec 을 만든 뒤 비충전형 경로에서 `ConsumeItemsByDefinition(ItemDef, 1)`(`:561`) 을 돌리는데, 그 슬롯이 마지막 스택이면 엔트리 제거 + `RemoveReplicatedSubObject` 로 인스턴스를 향한 강한 참조가 전부 사라진다(Outer 는 인스턴스가 소유자를 잡는 방향이라 소유자가 인스턴스를 살려두지 않는다). `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 이므로 다음 GC 이후 null 이 된다. 즉시 적용 GE 는 무해하지만, Duration/Periodic GE 를 붙인 사용 아이템에서는 "인스턴스 단위 데이터 추적"이라는 주석의 목적(`:524`, `:542`)이 주기 실행 시점에 깨지고, 클라이언트로 복제된 컨텍스트도 SourceObject 를 해석하지 못한다. 현재 유일한 사용 아이템은 충전형(에스트병)이라 인스턴스가 남으므로 아직 드러나지 않는다.
- **제안**: SourceObject 로 인스턴스가 아니라 `UWxItemDefinition`(에셋이라 수명이 안정적) 을 넣거나, 인스턴스 단위 추적이 필요하면 소비로 사라지는 경로에서는 즉시 적용 GE 만 허용하도록 규약을 명시한다.
- **확신도**: 중간

### 4. 🟡 `AddItemDefinition` 의 엔트리 생성 루프에 상한이 없다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:317-335`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:122-123`
- **범주**: 성능/안전
- **문제**: `ChunkCount = FMath::Min(MaxStack, Remaining)`(`:319`) 이 0 이면 `Remaining` 이 줄지 않아 `while (Remaining > 0)` 가 무한 루프에 빠지며, 매 반복 `NewObject` + `AddReplicatedSubObject` 를 수행해 서버가 그대로 멈춘다. `MaxStack` 의 `ClampMin = "1"` 은 디테일 패널 입력만 막을 뿐 값 자체를 보증하지 않는다(`MaxStack` 은 `:283` 에서 폴백 없이 그대로 쓰인다). 상한이 없는 것은 수량 쪽도 마찬가지라, 비스택 아이템에 큰 `Quantity` 를 준 보상 Row 하나가 그 수만큼의 UObject·복제 서브오브젝트를 만든다(`FWxItemRewardEntry::Quantity` 는 `ClampMin` 만 있고 `ClampMax` 가 없다 — `Public/Items/WxRewardTableRow.h:26`).
- **제안**: 루프 진입 전에 `const int32 SafeMaxStack = FMath::Max(1, MaxStack);` 로 정규화하고, 생성할 엔트리 수에 방어 상한 + `ensureMsgf` 를 둔다.
- **확신도**: 중간

### 5. 🟡 `AddItemDefinition` 이 인덱스 기반 루프 도중에 델리게이트를 방송한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:290-314`
- **범주**: 설계/구조
- **문제**: 내부 배열 참조를 캐시한 상태로 루프를 돌면서 매 반복 `NotifySlotChangedFromList`/`NotifyStackChangedFromList`(`:307-308`) 로 외부 구독자에게 제어를 넘기고, 다음 반복에서 다시 `EntryIndex` 로 슬롯을 접근한다. 구독자가 방송 처리 중 인벤토리를 변경하면(획득 즉시 자동 소비, 보상 회수 등) `ConsumeByDefinition` 의 `It.RemoveCurrent()`(`:194`) 로 엔트리가 앞당겨져 `EntryIndex` 가 다른 슬롯을 가리키고 잔여분이 엉뚱한 슬롯에 더해진다. `:317-335` 의 신규 엔트리 루프도 `AddEntry` → 방송 → 다음 `AddEntry` 순서라 같은 재진입 창을 갖고, `PreReplicatedRemove`(`:62-76`) 역시 `Entry` 참조를 잡은 채 방송한다. 현재 구독자(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `WxViewModel_Item.cpp`) 는 인벤토리를 읽기만 해 아직 재현되지 않는 잠복 위험이다.
- **제안**: 변경을 전부 적용해 `FWxInventoryChangeResult` 목록으로 모은 뒤 루프 밖에서 일괄 통지한다 — `ConsumeItemsByDefinition`(`:405-418`) 이 이미 쓰는 "변경 후 통지" 형태와 맞춘다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟡 StateTree 태스크 2종이 대상을 0번 PlayerController 로 고정한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:36`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:32`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 서버 권위 게이트 뒤에서 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 대상을 고른다. 모듈 전체가 서버 권위 + FastArray 복제로 멀티를 전제해 설계돼 있는데, 이 지점만 접속 순서상 첫 PC 로 귀결되어 상자를 연 플레이어가 아닌 다른 플레이어가 재화를 받거나 에스트병을 리필받는다(데디케이티드 서버에는 애초에 "로컬 플레이어"가 없어 리필 태스크 주석의 전제 자체가 성립하지 않는다). 기믹 StateTree 가 `Context.GetOwner()`(기믹 액터) 만 알고 상호작용 주체를 모르는 것이 근본 원인이다.
- **제안**: 기믹 측이 `OnInteracted(Interactor, ...)` 로 받은 주체를 StateTree 파라미터/글로벌 데이터로 노출해 태스크가 그 액터를 대상으로 삼게 한다. 싱글 전제로 유지할 거라면 두 태스크 주석에 "싱글 전용"임을 못박고 멀티 전환 시 손볼 지점으로 남긴다.
- **확신도**: 중간

### 7. 🟡 장비 경로 전체가 호출부 0건인 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:596-622`, `:357-389`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`(전체)
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` → `UWxEquipmentComponent::EquipItem` 을 부르는 코드가 저장소 전체에 없고 `BlueprintCallable` 도 아니라 BP 진입도 불가하다(헤더 `Public/Inventory/WxEquipmentComponent.h:28-30` 에 스스로 명시). `RemoveItemInstance` 도 호출부 0건이다. 결과적으로 `EquippedItemDef` 는 항상 null 이고, `AWxCharacterBase` 가 `OnEquipVisualChanged` 를 구독해도(`Source/WxGame/Character/WxCharacterBase.cpp:82`) 방송이 발생하지 않으며 EquipEffect GE 도 적용되지 않는다. 약 220줄이 컴파일·복제 등록만 소모하면서 "장비 시스템이 있다"는 오해를 만든다.
- **제안**: 트리거(UI 슬롯 → 어빌리티/서버 RPC → `EquipItemByDef`) 를 붙여 경로를 닫거나, 착수 시점이 멀면 컴포넌트째 제거하고 필요할 때 되살린다. 배선할 때 늦게 relevant 해진 클라가 초기 RepNotify 를 놓치는 문제(현재 상태를 되물을 pull API 부재, 헤더 `:30` 에 기록됨) 도 함께 처리한다.
- **확신도**: 높음(사실 확인 완료 — 조치 방향은 결정 사항)

### 8. 🟢 `GrantReward` 가 `World` 널 검증 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:38`, `:70`
- **범주**: 성능/안전
- **문제**: `SourceActor->GetWorld()` 결과를 검사 없이 `World->SpawnActorDeferred` 에 쓴다. `HasAuthority()` 는 월드 유무를 보장하지 않으며, 이 함수는 `BlueprintCallable` 로 노출된 모듈의 공개 진입점이라 호출 맥락을 통제할 수 없다.
- **제안**: 이른 반환 가드 한 줄을 추가한다.
- **확신도**: 높음

### 9. 🟢 `GetPrimaryAssetId` 가 등록되지 않은 PrimaryAssetType 을 반환한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12-15`
- **범주**: 설계/구조
- **문제**: `FPrimaryAssetId(TEXT("WxItem"), GetFName())` 을 반환하지만 `Config/DefaultGame.ini:46-50` 의 `PrimaryAssetTypesToScan` 에는 `Map`/`PrimaryAssetLabel`/`GameFeatureData`/`WxExperienceDefinition`/`WxExperienceActionSet` 만 있고 `WxItem` 이 없으며, 커스텀 `UAssetManager` 서브클래스도 설정돼 있지 않다. AssetManager 가 아이템 정의를 프라이머리 애셋으로 등록하지 않으므로 이 오버라이드는 무효이고, 쿠킹 규칙·청크·번들을 붙일 수단이 없으며 나중에 `LoadPrimaryAsset("WxItem", ...)` 를 쓰는 코드가 조용히 실패한다.
- **제안**: 아이템 폴더를 스캔하는 `WxItem` 타입을 `PrimaryAssetTypesToScan` 에 추가하거나, 관리 의도가 없다면 오버라이드를 제거해 `UPrimaryDataAsset` 기본 동작을 쓴다. 2번(선로드) 을 번들로 풀 거라면 이쪽이 선행 조건이다.
- **확신도**: 중간

### 10. 🟢 클라이언트 슬롯 순서가 서버와 어긋난다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:194`, `:421-437`, `:655-681`
- **범주**: 설계/구조
- **문제**: 서버는 `It.RemoveCurrent()`(내부적으로 `TArray::RemoveAt`) 로 순서를 보존하며 슬롯을 지우지만, 클라이언트의 FastArray 수신 경로는 `Items.RemoveAtSwap` 으로 지운다(UE 5.8 `FastArraySerializer.h:1193`). 제거가 한 번이라도 일어나면 클라의 `Entries` 순서가 서버와 달라지고, 순서에 의존하는 `FindFirstItemStackByDefinition`·`FindUsableInstance` 가 양쪽에서 다른 인스턴스를 고를 수 있다. 지금은 동일 ItemDef 인스턴스가 사실상 하나뿐이라 표시가 어긋나지 않지만, 슬롯 격자 UI 나 인스턴스별 상태(충전량 차이) 가 생기면 클라 표시가 서버와 갈린다.
- **제안**: 슬롯 순서를 UI 계약으로 삼을 거라면 엔트리에 명시적 정렬 키를 두고 표시 측이 그 키로 정렬한다. 순서를 계약으로 삼지 않는다면 헤더 주석에 "슬롯 순서는 서버·클라가 다를 수 있다"를 명시한다.
- **확신도**: 중간

### 11. 🟢 StateTree 태스크 헤더의 `GetInstanceDataType()` 인라인 정의 (코딩 규칙 6)
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h:49`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h:33`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 6(인라인 함수 정의 금지) 에 해당한다. 두 헤더 모두 예외 사유 주석을 달았지만(`:13`, `:12`), 같은 모듈의 `FindFragmentByClass<T>` 템플릿과 달리 이 함수는 일반 virtual 이라 `.cpp` 로 내리는 데 아무 제약이 없다 — 규칙이 실제로 강제되지 않는 유일한 지점이다.
- **제안**: `.cpp` 로 내려 규칙을 지키거나, 엔진 StateTree 관례를 따르기로 했다면 CLAUDE.md 에 "StateTree 노드의 `GetInstanceDataType()` 은 예외"로 명문화해 파일마다 사유 주석을 반복하지 않게 한다.
- **확신도**: 중간

### 12. 🟢 픽업 프롬프트에만 키 표기가 하드코딩돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:113-114`
- **범주**: 버그/정확성
- **문제**: 프롬프트 문구에 `"[F] {0}"` 로 키 글리프를 박아, 키 리바인딩이나 게임패드에서 실제 입력과 다른 표기가 뜬다. 계약 구현체 넷 중 이 파일만 그렇다 — `AWxEnemyCharacter`(`Source/WxGame/Character/WxEnemyCharacter.cpp:138-140`), `UWxDialogueComponent`(`Plugins/WxDialogue/.../WxDialogueComponent.cpp:50-53`), `UWxGimmickStateTreeComponent`(`Plugins/WxWorld/.../WxGimmickStateTreeComponent.cpp:155-163`) 는 모두 키 없는 순수 문구만 답한다. 스캐너가 여러 프롬프트를 한 목록에 모아 표시하므로(`Plugins/WxWorld/.../WxInteractionScannerComponent.cpp:84`) 행마다 표기가 갈리는 것도 보인다.
- **제안**: 키 표기는 HUD 쪽에서 Enhanced Input 바인딩으로 붙이고, 대상은 다른 구현체처럼 순수 문구만 답한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`
- **미검토 / 한계**:
  - 규칙 위반은 기계적으로 전수 확인했다 — 소스 22파일 전부 첫 줄 저작권 표기 일치, 람다 0건, `FORCEINLINE` 0건, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 한 곳뿐, `WxCore` 외 Wx 플러그인 의존 없음(`WxInventory.Build.cs`·`.uplugin` 양쪽), `Wx` prefix 누락 없음, override 의 `Super::` 호출 누락 없음, 델리게이트 바인딩 콜백 자체가 모듈 내에 없어 `Handle` prefix 규칙은 해당 없음. 남은 위반은 11번 하나다.
  - 이번 판은 직전 리뷰(커밋 `e9440f73`) 이후 코드 변경이 주석 정리뿐이라, 기존 11건을 현재 라인 번호로 전부 재검증해 유지하고 12번(픽업 프롬프트 키 표기) 을 새로 추가한 갱신본이다. 재검증 과정에서 두 결론을 엔진 소스로 다시 확인했다 — (1) 소유 액터가 등록 서브오브젝트 목록을 쓰지 않아도 컴포넌트의 목록은 `UActorChannel::ReplicateSubobject(UActorComponent*)` 갈래가 처리하므로(`DataChannel.cpp:4338-4388`) 아이템 인스턴스 복제는 정상이고, (2) 클라 FastArray 제거는 `RemoveAtSwap` 이라 순서가 어긋난다(10번). 다만 둘 다 코드 대조 산출이며 실제 네트워크 세션 재현은 하지 않았다.
  - BP/WBP 및 데이터 자산 내부(`BP_ItemPickup`, `DA_*` 의 Fragment 실제 조합, RewardRow 실데이터) 는 범위 밖이라 확인하지 않았다 — 1번(다중 픽업 드랍) 과 4번(대량 Quantity·`MaxStack` 0) 의 실제 발현 여부는 데이터에 달려 있다.
  - 아이템 인스턴스가 PlayerController 수명에 묶여 심리스 트래블·PC 재생성 시 인벤토리가 사라지는지는 세이브/로드(`WxSave`) 와 함께 봐야 하는 주제라 이번 범위에서도 다루지 않았다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 22파일 — `/module-review`로 갱신*
