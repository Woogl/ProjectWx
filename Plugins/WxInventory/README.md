# WxInventory — 아이템/인벤토리 시스템

> 아이템의 정적 정의(데이터 자산)와 런타임 인스턴스를 관리하고, 액터에 부착되는 인벤토리 매니저 컴포넌트로 추가·차감·사용·장착을 권한(서버) 주도로 처리해 FastArray 로 클라이언트에 동기화한다.

## 책임
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 아이템의 속성/행동 선언, 런타임 인스턴스(`UWxItemInstance`) 수명 관리
- 인벤토리 컬렉션의 추가/머지/분할·차감·소비·충전(에스트병)·장착 요청 및 변경 브로드캐스트(`UWxInventoryManagerComponent`)
- 보상 지급용 DataTable Row 구조체(`FWxRewardTableRow`) 제공
- 장착의 실제 시각/부착 반영은 담당하지 않는다 — `IWxEquipmentInterface` 로 게임 측(Character)에 위임한다(플러그인 역참조 회피)
- UI 출력·입력 처리(WxUI), 픽업 액터 스폰/배치(WxWorld) 자체는 담당하지 않는다 — 데이터(Fragment)만 제공한다

## 의존성
- **주요 의존**: `WxCore`(공용 정의), `GameplayAbilities`(사용/장착 시 GameplayEffect 적용), `Niagara`(픽업 비주얼 Fragment 데이터), `GameplayTags`, `NetCore`(FastArray 리플리케이션), `DeveloperSettings`
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 액터 부착 인벤토리 매니저. Add/Consume/Use/Equip/Refill 진입점, 변경 델리게이트 발행 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` | 엔트리 컬렉션. FastArray 로 머지/분할/차감과 효율 레플리케이션 수행 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Fragment 컴포지션·등급·카테고리 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 아이템 한 자루의 런타임 인스턴스. 충전량 보유, GA SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | Fragment 베이스(EditInline). `OnInstanceCreated` 로 초기 상태 주입 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemFragment_Equippable` | 장착 메시/소켓/EquipEffects 선언 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemFragment_Usable` | 사용 시 적용할 GameplayEffect | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemFragment_Charges` | 인스턴스별 충전 횟수(에스트병). MaxCharges | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemFragment_Stackable` | 스택 가능·MaxStack 선언(부재 시 1슬롯=1개) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemFragment_PickupVisual` | 픽업 액터 외형 데이터(메시/Niagara) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `IWxEquipmentInterface` | 장착 요청을 게임 측에 위임하는 인터페이스 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentInterface.h` |
| `FWxRewardTableRow` | 보상 지급용 DataTable Row((아이템,수량) 최대 5쌍) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 내부 구조
- `Public/Inventory` — 인벤토리 매니저 컴포넌트·FastArray 리스트·엔트리, 장착 위임 인터페이스
- `Public/Items` — 아이템 정의/인스턴스/Fragment 계층, 보상 테이블 Row

## 확장 포인트 / 규약
- **새 아이템 행동 추가**: `UWxItemFragment` 를 상속한 Fragment 를 만들고, 인스턴스 초기 상태가 필요하면 `OnInstanceCreated` 를 오버라이드한다. `UWxItemDefinition::Fragments`(Instanced) 에 EditInline 으로 부착한다.
- **데이터 주도**: 아이템은 `UWxItemDefinition` 데이터 자산으로 정의하며, `Category`(`EWxItemCategory`) 가 UI/기능 1차 분기 축이고 행동은 Fragment 조합이 결정한다. 보상은 `FWxRewardTableRow` DataTable(RowName 예: `Reward_Quest_01`) 로 (아이템, 수량) 을 정의한다.
- **Stackable 머지/분할**: `Stackable` Fragment 가 있으면 MaxStack 한도까지 기존 엔트리에 머지하고 초과분은 새 엔트리로 분할, 부재 시 항상 1슬롯=1개.
- **충전형(Charges)**: 사용 시 인벤토리 스택이 아니라 인스턴스 충전량을 1 감소(아이템은 인벤토리에 잔존), `RefillItemCharges` 로 MaxCharges 회복. 회복 효과는 `Usable` Fragment 와 함께 부착해야 발생한다.
- **장착**: `EquipItemByDef` 는 스택을 차감하지 않고 소유 폰의 `IWxEquipmentInterface::EquipItem` 으로 위임한다(nullptr 이면 장착 해제).
- **리플리케이션/권한(최대 4인 멀티)**: Add/Consume/Use/Equip/Refill 은 권한(서버)에서만 호출되어야 하며, `FWxInventoryList`(FastArray)와 SubObject 로 등록된 `UWxItemInstance` 가 클라이언트에 동기화된다. 변경 통지는 서버 변경 경로와 클라이언트 복제 콜백이 `NotifyXxxFrom...` 진입점으로 수렴해 `OnInventoryStackChanged`/`OnInventorySlotChanged`/`OnInventoryChargeChanged` 를 발행한다. 인벤토리는 PlayerController 에 부착되며 `FindInventory` 로 폰→컨트롤러를 거쳐 조회한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 모듈의 핵심 진입점. 엔트리/리스트/매니저와 머지·차감·사용·충전·장착·통지 규약이 모두 헤더 주석에 정리되어 있다.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템 행동을 결정하는 Fragment 계층. 어떤 Fragment 가 어떤 동작을 켜는지 한눈에 파악된다.
3. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` — 정의/카테고리/등급과 Fragment 조회 API.

## 관련
- 상위: 게임 측(`WxGame`)이 인벤토리 컴포넌트를 PlayerController 에 부착하고 `IWxEquipmentInterface` 를 구현. 사용/장착 GameplayEffect 는 [[WxCombat]] 의 GAS 와 맞물린다. 픽업/보상은 [[WxWorld]]·[[WxQuest]] 가 데이터를 소비한다.

---
*문서 기준 커밋 `03157fe2` · 생성일 2026-06-09 · 소스 12파일 — `/readme-writer`로 갱신*
