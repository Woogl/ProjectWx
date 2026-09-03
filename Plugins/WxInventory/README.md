# WxInventory — 아이템·인벤토리 시스템

> 아이템 정의(데이터 자산)와 런타임 인스턴스를 나누어 관리하고, PlayerController 에 붙는 인벤토리 컴포넌트로 아이템의 생성·소비·장착·복제를 서버 권한에서 관장한다. 보상 지급과 픽업 드랍의 진입점도 여기 있다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 "이 아이템이 무엇을 할 수 있는가"를 데이터 주도로 기술
- 인벤토리 슬롯의 추가/머지/소비/충전을 FastArray(`FWxInventoryList`)로 서버→클라 복제
- 사용 아이템의 GameplayEffect 적용과 스택/충전 차감(에스트병식 Charges 포함)
- 보상 테이블(`FWxRewardTableRow`) 지급 — 픽업 액터 스폰·발사 또는 인벤토리 직접 지급
- 장착 아이템의 GE 라이프사이클·외형 변경 방송(`UWxEquipmentComponent`)

**경계 (비담당)**
- 무기 액터 스폰/메시 스왑·소켓 부착: 장비 컴포넌트는 `OnEquipVisualChanged` 로 메시/소켓만 방송하고, 실제 반영은 게임 모듈(캐릭터의 `WeaponActor`)이 한다
- 인벤토리 컴포넌트의 액터 부착: 코드가 아니라 Experience 에셋의 컴포넌트 주입 설정으로 이뤄진다
- 상호작용(스캐너·프롬프트) 계약: `IWxInteractable` 은 [[WxCore]] 소유, 실제 상호작용 주체는 [[WxWorld]]
- UI 표시: 델리게이트만 발행하고 뷰모델/위젯은 [[WxUI]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 모듈 허브. Add/Consume/Use/Refill/Equip 의 서버 권한 API + 변경 델리게이트 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). `Fragments` 로 기능을 조합 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 기능 축 베이스. Equippable/Usable/Charges/Stackable/Pickup/Grade 파생 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 런타임 개별 아이템. 충전량 상태 보유, 슬롯 델리게이트의 안정 식별자 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관/복제 + EquipEffect GE 관리, 외형 변경 방송 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 지급의 서버 권위 진입점(무상태 라이브러리) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 아이템/재화 드랍 픽업 액터. 상호작용 시 인벤토리 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(최대 5개 항목). Item 은 지급 시점 지연 로드 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템 기능**은 `UWxItemFragment` 를 상속해 만들고 `UWxItemDefinition::Fragments` 에 EditInline 인스턴스로 부착한다. 인스턴스 초기 상태 주입이 필요하면 `OnInstanceCreated` 를 오버라이드한다(예: `Charges`). 카테고리 분류는 `UWxItemDefinition::Category`(`EWxItemCategory`)가, 기능은 Fragment 가 담당하는 직교 축이다.
- **데이터 주도**: 아이템은 `UWxItemDefinition` 자산, 보상은 `FWxRewardTableRow` DataTable 로 구동된다. 소비/장착 효과는 Fragment 의 `TSubclassOf<UGameplayEffect>` 로 지정한다.
- **StateTree 태스크** 2종이 보상/충전 흐름을 노드로 노출한다: `FWxStateTreeTask_GiveRewards`(보상 지급), `FWxStateTreeTask_RefillItemCharges`(로컬 플레이어 충전 리필). 둘 다 초기 진입(복원/레이트조인)에서는 중복 방지를 위해 실행하지 않는다.
- **리플리케이션/권한**: Add/Consume/Use/Refill/Equip 은 모두 서버 권한 전용이며, `FWxInventoryList`(FastArraySerializer)로 델타 복제된다. 충전량(`CurrentCharges`)과 장착 정의(`EquippedItemDef`)는 OnRep 으로 클라 통지된다. 관찰자가 인벤토리보다 먼저 존재할 수 있어 준비 신호는 인스턴스가 아닌 `UWxInventoryComponent::OnAnyInventoryReady`(정적)로 발행된다.
- 장비 경로(`EquipItemByDef`→`EquipItem`)는 배선만 있고 현재 호출부가 없어 비활성이다 — 헤더 주석의 "미구현" 표기 참조.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 모듈의 모든 제어 흐름이 모이는 허브. API 주석에 권한/원자성/충전 규칙이 정리돼 있다.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 어떤 축으로 확장되는지 한눈에. Definition↔Instance 분리의 이유가 여기 담긴다.
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp` — 머지/분할 스택, 정의 단위 합산 소비, FastArray 통지 배선의 실제 구현.

## 관련
- 상위: 보상/충전 태스크는 [[WxAI]]·Experience 의 StateTree 에서 호출된다. 델리게이트 소비는 [[WxUI]], 픽업 상호작용은 [[WxWorld]], 사용/장착 GE 는 [[WxCombat]] 및 GAS 와 맞물린다. 공용 계약(`IWxInteractable` 등)은 [[WxCore]].

---
*문서 기준 커밋 `f0aad4c` · 생성일 2026-09-03 · 소스 22파일 — `/readme-writer`로 갱신*
