# WxInventory — 아이템·인벤토리 시스템

> 아이템 정의(데이터 자산)와 런타임 인스턴스, 플레이어 인벤토리 보유·사용·장착, 그리고 보상 지급을 담당한다. 인벤토리는 PlayerController 에 붙어 FastArray 로 복제된다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 조합으로 아이템의 데이터·기능을 컴포지션
- 런타임 아이템 인스턴스(`UWxItemInstance`)의 생성·소멸·복제 및 충전량 상태
- 플레이어 인벤토리 보유/추가/소비/사용, 슬롯·합계·충전 단위 변경 통지(델리게이트)
- 보상 테이블(`FWxRewardTableRow`) 지급과 픽업 액터(`AWxItemPickup`) 스폰/발사
- Experience 시작 아이템 지급(GameFeatureAction), 보상·리필 StateTree 태스크

**경계 (비담당)**
- GameplayEffect 적용 대상 ASC·`Ability.UseItem` 같은 태그·어빌리티 정의는 [[WxCore]] 및 소유 폰의 GAS 쪽 소관 (여기선 태그로 발동만)
- 픽업이 스캐너에 잡히는 상호작용 계약(`IWxInteractable`)은 [[WxCore]]
- Experience 에셋·GameFeature 활성 흐름 자체는 [[WxGame]]/GameFeature 계층

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 인벤토리 핵심. Add/Consume/Use/Equip 전부 여기 진입. PlayerController 부착 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `FWxInventoryList` | FastArray 복제 백엔드. 엔트리 추가·차감의 실제 구현체 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 정적 아이템 정의(PrimaryDataAsset) + Fragment 컬렉션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 슬롯 단위 런타임 상태(충전량·식별자·GE SourceObject) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxRewardLibrary` | 보상 지급의 서버 권위 진입점(픽업 스폰/직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `UWxGameFeatureAction_AddInventoryItems` | Experience 켜질 때 시작 아이템 지급 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxGameFeatureAction_AddInventoryItems.h` |
| `AWxItemPickup` | 월드 픽업 액터. 상호작용 시 인벤토리에 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- 새 아이템 기능 추가는 `UWxItemFragment` 를 상속해 `UWxItemDefinition::Fragments` 에 EditInline 부착. 카테고리는 `Category`(enum), Fragment 는 "무엇을 할 수 있나"만 책임. 필요 시 `OnInstanceCreated` 로 인스턴스 초기 상태 주입
- 데이터 주도: 아이템은 `UWxItemDefinition` 자산, 보상은 `FWxRewardTableRow` DataTable, 시작 아이템은 Add Inventory Items GameFeatureAction
- 권한 모델: Add/Consume/Use/Equip/Grant 는 모두 서버 권한 전용. 클라는 `FWxInventoryList`(FastArray)·`UWxItemInstance`(OnRep) 복제로 수렴하며, 변경 통지는 `OnInventoryStackChanged`/`SlotChanged`/`ChargeChanged`/`ContentsChanged` 델리게이트로 관찰
- 인벤토리 부착은 코드가 아니라 Experience 주입 설정으로 하며, 관찰자는 클래스 차원의 `OnAnyInventoryReady`/`OnAnyInventoryEnded` 로 존재를 감지
- 장비 경로(`EquipItemByDef`/`UWxEquipmentComponent`)는 배선만 있고 트리거 호출부가 아직 없는 미구현 상태

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 모듈의 관문. 보유/사용/소비 API와 복제·통지 구조가 한눈에 들어온다
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 6종으로 아이템 데이터 모델 전체를 파악
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp` — AddItemDefinition 머지/분할, Consume 원자성, Use/Charges 처리의 실제 로직
4. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 보상이 픽업 스폰과 직접 지급으로 갈리는 지점

## 관련
- 상위: Experience/GameFeature 계층([[WxGame]])이 인벤토리 주입·시작 아이템 지급을 구동, 아이템 사용은 소유 폰의 GAS([[WxCombat]]/[[WxCore]])로 이어짐. 공용 정의·상호작용 계약은 [[WxCore]]

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 24파일 — `/readme-writer`로 갱신*
