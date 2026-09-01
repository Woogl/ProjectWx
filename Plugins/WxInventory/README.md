# WxInventory — 아이템 · 인벤토리 시스템

> 데이터 주도 아이템 정의(Definition + Fragment)와 서버 권위 인벤토리를 제공한다. PlayerController에 부착되는 인벤토리 컴포넌트가 아이템 인스턴스의 생성·소비·복제를 관장하고, 장비·보상·픽업·StateTree 태스크가 그 위에 얹힌다.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`)와 기능 Fragment 컴포지션, 런타임 인스턴스(`UWxItemInstance`) 수명 관리
- PlayerController 부착 인벤토리(FastArray 복제, 추가/소비/사용/충전), 정의·슬롯·충전 단위 변경 통지
- 장비 착용 GE 라이프사이클과 외형 반영 방송, 보상 지급(픽업 스폰 또는 직접 지급)과 픽업 액터 상호작용

**경계 (비담당)**
- 아이템 사용/장착 GE의 실제 능력 발동·적용은 GAS(어빌리티/이펙트) — 무기 외형(메시 스왑·소켓)의 실제 반영은 게임 모듈([[WxGame]])이 방송을 받아 수행
- 픽업이 스캐너에 잡히는 상호작용 계약은 [[WxCore]]의 `IWxInteractable`
- `Ability_UseItem` 등 Gameplay Tag는 [[WxCore]]에서 선언된 것을 소비

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | PlayerController 부착. 아이템 추가·소비·사용·충전·복제의 서버 권위 진입점 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Category + Fragment 컬렉션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 개별 아이템의 런타임 가변 상태(충전량 등)·안정 식별자 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxEquipmentComponent` | Pawn 부착. 장착 ItemDef 복제와 EquipEffect GE 관리, 외형 변경 방송 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상(`FWxRewardTableRow`) 지급의 서버 권위 진입점(픽업 스폰/직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 상호작용 시 인벤토리에 지급 후 파괴되는 픽업 액터 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxRewardTableRow` | 보상 DataTable 로우(지연 로드되는 아이템 항목 5개) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 자산을 만들고 `Category`를 지정한 뒤 필요한 기능을 `Fragments`에 EditInline으로 조합한다. 카테고리는 분류(`Category`), Fragment는 "무엇을 할 수 있는가"(기능 축)로 서로 직교한다.
- **새 기능**: `UWxItemFragment`를 상속해 데이터·행동을 추가한다. `OnInstanceCreated`로 인스턴스 초기 상태를 주입한다(예: `UWxItemFragment_Charges`가 `MaxCharges`로 시드). 소비처는 `FindFragmentByClass<T>()`로 프래그먼트를 조회한다.
- **데이터 주도**: 보상은 `FWxRewardTableRow` DataTable로 기술하며 아이템은 `TSoftObjectPtr`라 지급 시점에 동기 로드된다. StateTree 태스크(`FWxStateTreeTask_GiveRewards`/`_RefillItemCharges`)가 라이브 전이에서 이를 구동한다(초기 진입/복원/레이트조인은 중복 방지로 스킵).
- **리플리케이션**: 인벤토리는 `FWxInventoryList`(FastArraySerializer)로 델타 복제되고, `UWxItemInstance`는 서브오브젝트로 복제 등록된다. Add/Consume/Use/Equip/Refill 등 변경은 **서버 권한에서만** 호출해야 한다. 클라 관찰은 `OnAnyInventoryReady`(클래스 차원) + 정의/슬롯/충전 델리게이트로 수렴한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 인벤토리의 공개 표면과 `FWxInventoryList` FastArray 계약, 권위/통지 경로가 모두 여기 모여 있다.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템의 기능이 어떻게 컴포지션되는지(6종 Fragment)를 한눈에 본다.
3. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 아이템이 세계로 나가는 지급/드랍 경로의 진입점.

## 관련
- 상위: [[WxGame]] (Experience 주입으로 컴포넌트 부착, 무기 외형 반영), GameFeature 콘텐츠 플러그인
- 의존 foundation: [[WxCore]]

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 22파일 — `/readme-writer`로 갱신*
