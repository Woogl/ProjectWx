# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(Fragment 컴포지션)와 런타임 인스턴스, PlayerController 부착형 인벤토리, 사용·충전·장비·보상 지급까지 아이템 수명 전체를 서버 권위로 관장하는 도메인 플러그인. 인벤토리 상태는 FastArray 로 서버→클라 복제된다.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`)와 Fragment 컴포지션(Stackable/Usable/Charges/Equippable/Pickup/Grade)
- 런타임 인스턴스(`UWxItemInstance`) 생성·소멸·복제와 인스턴스별 충전량(에스트병 방식)
- 인벤토리 소유·머지·차감·사용(`UWxInventoryManagerComponent`)과 정의/슬롯/충전 단위 변경 델리게이트
- 보상 지급 진입점(`UWxRewardLibrary::GrantReward`)과 DataTable Row(`FWxRewardTableRow`)
- 월드 아이템 픽업 액터(`AWxItemPickup`, `IWxInteractable` 자체 구현)
- 장비 상태 보관·복제와 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`) — 단, 트리거 미배선(아래 참조)
- 보상/충전 리필을 라이브 전이에서 발동하는 StateTree Task 노드

**경계 (비담당)**
- 소비/사용의 실제 판정·발동은 소유 폰의 GameplayAbility(UseItem)가 수행 — `RequestUseConsumable`은 AssetTag 로 발동만 함 (GAS 어빌리티)
- 무기 외형 반영(메시 스왑/소켓 재부착)은 `OnEquipVisualChanged` 방송만 하고 실제 반영은 게임 모듈이 담당 [[WxGame]]
- 상호작용 스캔/프롬프트 표시 파이프라인 [[WxWorld]] (픽업은 `IWxInteractable` 계약만 구현)
- 인벤토리 뷰 표시 [[WxUI]] (매니저는 클래스 차원 `OnAnyInventoryReady`·변경 델리게이트로 알림만)

## 의존성
- **주요 의존**: [[WxCore]](`IWxInteractable`, `WxGameplayTags`), 엔진 서브시스템 `GameplayAbilities`(ASC/GE), `ModularGameplay`(`UControllerComponent` 주입), `StateTree`, `NetCore`(FastArray/SubObject 복제), `Niagara`(픽업 이펙트, Private)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (uplugin·Build.cs 모두 Wx 중 WxCore 하나만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | PlayerController 부착 인벤토리 매니저. Add/Consume/Use/Equip 서버 권위 진입점 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset) + Fragment 컬렉션 + 카테고리 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | Fragment 베이스 + 구체 6종(Stackable/Usable/Charges/Equippable/Pickup/Grade) | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 아이템 런타임 인스턴스. 충전량 보관, GA SourceObject | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | FastArray 슬롯 컬렉션. 머지/분할/차감 로직 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxRewardLibrary` | 보상 지급 BP 라이브러리(`GrantReward`) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` / `FWxItemRewardEntry` | 보상 DataTable Row(최대 5항목) | `Source/WxInventory/Public/Items/WxRewardTableRow.h` |
| `AWxItemPickup` | 월드 픽업 액터(`IWxInteractable`) | `Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxEquipmentComponent` | 장비 상태·GE 라이프사이클(폰 부착) | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`(EWxItemCategory) 지정 + 필요한 Fragment 를 EditInline 으로 부착. 카테고리(분류축)와 Fragment(기능축)는 직교한다.
- **새 기능 축**: `UWxItemFragment` 파생 UCLASS(`DefaultToInstanced, EditInlineNew`)를 추가. 인스턴스 초기화가 필요하면 `OnInstanceCreated` 오버라이드. 조회는 `FindFragmentByClass<T>()`.
- **스택 규칙**: Stackable Fragment 있으면 `MaxStack`까지 한 슬롯 머지·초과분 분할, 없으면 1슬롯=1개.
- **충전형(에스트병)**: Charges + Usable 함께 부착. 사용 시 스택이 아니라 인스턴스 충전량 1 감소, 리필로 `MaxCharges` 회복.
- **보상**: `FWxRewardTableRow` Row 작성(RowName 예 `Reward_Quest_01`) 후 `GrantReward` 로 지급. Pickup Fragment 있으면 월드 드랍, 없으면 대상 인벤토리 직접 지급. StateTree 는 `Give Rewards`/`Refill Item Charges` Task 로 라이브 전이에서만 발동(복원·레이트조인 시 재실행 안 함).
- **리플리케이션 모델**: 인벤토리는 `FWxInventoryList`(FastArraySerializer), 인스턴스는 SubObject 복제로 클라 동기화. 모든 변경은 서버 권위이며 클라 복제 콜백이 서버 변경 경로와 같은 통지 진입점으로 수렴.
- **⚠️ 미구현(배선만 존재)**: 장비 경로(`EquipItemByDef`→`UWxEquipmentComponent::EquipItem`)는 호출부가 저장소 전무. `EquippedItemDef`는 항상 null, `OnEquipVisualChanged`/EquipEffects 미발동. `RemoveItemInstance`도 호출부 0건. 실사용하려면 UI→어빌리티/RPC→`EquipItemByDef` 트리거를 붙여 경로를 닫아야 한다.

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 전체 API·FastArray 슬롯 모델·통지 델리게이트가 한곳에 있는 허브
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 6종이 곧 아이템 기능의 어휘. 이걸 알아야 Definition 을 읽을 수 있다
3. `Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — 머지/분할/차감/사용/복제 등록의 실제 구현
4. `Source/WxInventory/Public/WxRewardLibrary.h` — 아이템이 세계로 들어오는 지급 경로(픽업 드랍 vs 직접 지급 분기)

## 관련
- 상위: [[WxGame]] (게임 모듈, 무기 외형 반영·어빌리티 소유, Experience 로 컴포넌트 주입)
- 계약: [[WxCore]] (`IWxInteractable`, `WxGameplayTags`)
- 소비처: [[WxUI]] (인벤토리 뷰), [[WxWorld]] (상호작용 스캔)

---
*문서 기준 커밋 `6e08d6d` · 생성일 2026-08-05 · 소스 22파일 — `/readme-writer`로 갱신*
