# WxInventory — 아이템·인벤토리 시스템

> 아이템 정의(데이터 자산)와 런타임 소유 상태를 관리한다. 인벤토리 보유·소비·장착, 보상 지급, 픽업 액터까지 아이템의 생애 전반을 책임진다.

## 책임
**담당**
- 아이템의 정적 정의(`UWxItemDefinition` + `UWxItemFragment` 컴포지션)와 런타임 인스턴스(`UWxItemInstance`) 관리
- PlayerController 부착 인벤토리의 추가·소비·사용·충전과 FastArray 복제
- 보상 테이블 기반 지급(직접 지급 / 픽업 액터 스폰·발사)
- 장비 슬롯 보관과 EquipEffect GE 라이프사이클(외형 반영은 방송만)
- StateTree 태스크로 보상 지급·충전 리필을 시퀀스에서 트리거

**경계 (비담당)**
- 무기 메시 스왑/소켓 재부착 등 외형 반영 — 델리게이트로 방송하고 [[WxGame]] 측이 수행
- 아이템 사용/장착 어빌리티 실행과 GameplayEffect 정의 — [[WxCombat]]/GAS
- 픽업 상호작용 스캔·트리거 — [[WxWorld]](계약 `IWxInteractable`은 [[WxCore]])
- 인벤토리 뷰 UI/HUD — [[WxUI]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 인벤토리 중심 허브. PlayerController에 부착되어 Add/Consume/Use/Refill과 복제·통지를 관장 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(`UPrimaryDataAsset`). Category + Fragments 조합으로 행동 구성 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스. 하위: Equippable/Usable/Charges/Stackable/Pickup/Grade | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 슬롯의 런타임 가변·복제 단위. 충전량 보유, 슬롯 델리게이트의 안정 식별자·GE SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `FWxInventoryList` | 인벤토리 컴포넌트 내부의 FastArraySerializer. 실제 엔트리 저장·머지·차감·복제 델타 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxRewardLibrary` | 보상 지급의 서버 권위 진입점(무상태 1회성). 픽업 스폰 또는 직접 지급 분기 | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 지급용 픽업 액터(`IWxInteractable`). 상호작용 시 인벤토리에 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxEquipmentComponent` | 폰 부착 장비 컴포넌트. 장착 ItemDef 보관·복제, EquipEffect 적용, 외형 방송 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |

## 확장 포인트 / 규약
- **새 아이템 행동**: `UWxItemFragment`를 상속해 정의 자산의 `Fragments`에 EditInline 부착. `OnInstanceCreated`로 인스턴스 초기 상태 주입. 카테고리(`EWxItemCategory`)와 직교하며 기능 축만 담당.
- **소비 아이템**: `Usable` Fragment의 `Effect`(GE)를 `UseItemByDef`가 적용+스택 차감. `Charges` Fragment를 겹치면 스택 대신 인스턴스 충전량으로 사용 가능 여부가 결정(에스트병 방식), `RefillItemCharges`로 회복.
- **데이터 주도 보상**: `FWxRewardTableRow`(DataTable Row, 최대 5개 항목) → `UWxRewardLibrary::GrantReward` 또는 StateTree `FWxStateTreeTask_GiveRewards`. `Item`은 SoftPtr라 지급 시점에 동기 로드.
- **리플리케이션/권한**: Add/Consume/Use/Equip 등 변경은 서버 권위 전용. `FWxInventoryList`(FastArray)로 클라 동기화, `UWxItemInstance`는 개별 복제 서브오브젝트. 관찰자용 통지는 `On*Changed` 델리게이트 + 클래스 차원 `OnAnyInventoryReady`.
- **미배선 주의**: 장비 경로(`EquipItemByDef`→`UWxEquipmentComponent::EquipItem`)는 배선만 있고 호출부가 없어 현재 항상 비활성. 여는 방법은 인벤토리·장비 컴포넌트 헤더 주석 참조.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 모듈의 허브. 인벤토리 API·통지 델리게이트·내부 `FWxInventoryList`가 한 파일에 모여 전체 흐름의 목차 역할.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 "무엇을 할 수 있나"를 정하는 Fragment 6종. 정의/인스턴스/사용 흐름이 여기서 갈린다.
3. `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp` — 보상 → 픽업 스폰/직접 지급 분기의 실제 구현. 아이템이 세상에 들어오는 경로.

## 관련
- 상위: 보상·픽업을 소비하는 [[WxWorld]]·[[WxQuest]] 및 시퀀스 로직(StateTree), 인벤토리를 표시·조작하는 [[WxUI]], 아이템 사용/장착 효과를 받는 [[WxCombat]](GAS). 부착은 GameMode가 고른 Experience 에셋의 컴포넌트 주입으로 이뤄진다.

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 22파일 — `/readme-writer`로 갱신*
