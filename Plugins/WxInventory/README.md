# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(데이터 자산)와 런타임 인스턴스를 나누어, 플레이어 인벤토리의 획득·소비·사용·리플리케이션과 보상 지급을 책임진다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 아이템의 데이터·행동을 기술
- 인벤토리 소유·복제: `UWxInventoryComponent`(PlayerController 부착) 위 FastArray(`FWxInventoryList`) 로 서버 권한 변경을 클라에 동기화
- 스택 머지/분할, 정의 단위 합산 소비, 충전형(에스트병) 사용·리필
- 보상 지급 경로: DataTable Row → 픽업 액터 스폰 또는 인벤토리 직접 지급, StateTree 태스크 진입점

**경계 (비담당)**
- 아이템 사용 효과의 실제 적용: GameplayEffect 를 소유 폰 ASC 에 거는 것은 GAS/[[WxCombat]] 어빌리티가 수행 (본 모듈은 GE Spec 검증·차감·적용 트리거만)
- 픽업을 잡는 상호작용·스캔: 계약(`IWxInteractable`)은 [[WxCore]], 스캐너 구현은 [[WxWorld]]
- 인벤토리 UI 표출: 델리게이트만 발행하고 뷰는 [[WxUI]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 모듈의 정면 진입점 — 획득/소비/사용/리필 API 와 변경 델리게이트를 소유 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `FWxInventoryList` | 컴포넌트 내부 FastArray. AddEntry/Consume 등 실제 슬롯 변경이 여기서 일어남 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 정적 정의(PrimaryDataAsset). Fragment 컬렉션을 담음 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 아이템 기능 축의 베이스 — Usable/Charges/Stackable/Pickup/Grade 파생 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 개별 아이템의 런타임 수명·식별·충전량 단위(복제 UObject) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxRewardLibrary` | 보상 Row 지급의 서버 권위 진입점(무상태 라이브러리) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` | 보상 DataTable 로우(최대 5항목, soft-ref 아이템) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |
| `AWxItemPickup` | 지급용 픽업 액터. `IWxInteractable` 구현, 상호작용 시 인벤토리에 지급 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- 새 아이템 행동을 추가하려면 `UWxItemFragment` 를 상속한 Fragment 를 만들고 정의 자산의 `Fragments` 에 EditInline 으로 부착한다. Category 분류는 Fragment 가 아니라 `UWxItemDefinition::Category` 가 표현한다.
- 데이터 주도: 아이템은 `UWxItemDefinition`(PrimaryDataAsset, AssetManager 등록), 보상은 `FWxRewardTableRow` DataTable 이 구동한다. 아이템 참조는 `TSoftObjectPtr` 라 지급 시점에 동기 로드된다.
- 리플리케이션/권한: 모든 Add/Consume/Use/Refill 은 서버 권한에서만 호출해야 하며, `FWxInventoryList`(FastArraySerializer) 로 클라에 델타 동기화된다. 인스턴스는 `ReadyForReplication` 경로로 서브오브젝트 등록된다.
- 시작 아이템은 컴포넌트 `StartingItems` 에 저작하고 `BeginPlay` 에서 권한 측이 1회 지급한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 모듈 API 전체와 변경 델리게이트(Stack/Slot/Charge/Contents)의 계약이 한자리에 모여 있다.
2. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp` — 스택 머지/분할·합산 소비·충전형 사용의 실제 제어 흐름.
3. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 "무엇을 할 수 있는가"를 규정하는 Fragment 5종의 데이터 모델.

## 관련
- 상위: [[WxCombat]](UseItem 어빌리티가 사용 효과를 적용), [[WxUI]](인벤토리 뷰), [[WxWorld]](픽업 상호작용 스캔). 공용 계약·기반은 [[WxCore]].

---
*문서 기준 커밋 `e0e3ecc` · 생성일 2026-09-09 · 소스 20파일 — `/readme-writer`로 갱신*
