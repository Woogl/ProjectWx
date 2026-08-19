# WxInventory — 아이템·인벤토리 시스템

> 아이템 정의(DataAsset) + Fragment 컴포지션으로 아이템의 데이터·행동을 조립하고, PlayerController 에 붙는 매니저 컴포넌트가 인스턴스의 생성·소멸·복제를 관장한다. 장착·사용(GAS 연동)·보상 지급·픽업까지 아이템 수명 전반을 담당한다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션 모델 — 카테고리(Equipment/Consumable/Currency)와 기능 축(Equippable/Usable/Charges/Stackable/Pickup/Grade)의 직교 분리.
- 인벤토리 상태의 서버 권한 변경·FastArray 복제(`FWxInventoryList`)와 인스턴스 단위 가변 상태(`UWxItemInstance`, 충전량 등).
- 아이템 사용(Usable/Charges → GE 적용), 리필(에스트병식 충전 회복).
- 보상 지급(`UWxRewardLibrary::GrantReward`) — 픽업 스폰/발사 또는 인벤토리 직접 지급.
- 장착 상태 보관·복제 및 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`).

**경계 (비담당)**
- 무기 외형 반영(메시 스왑·소켓 재부착): `UWxEquipmentComponent` 는 데이터만 방송(`OnEquipVisualChanged`)하고 반영은 게임 모듈(캐릭터 ChildActor) 측이 수행 — [[WxGame]].
- 상호작용 스캔·프롬프트 표시 흐름: 픽업은 계약 인터페이스 `IWxInteractable`(WxCore)만 구현 — [[WxWorld]].
- UseItem 어빌리티 실행 자체(입력 경로): 어빌리티 발동만 요청하며 판정·차감은 어빌리티가 수행 — [[WxCombat]].

## 의존성
- **주요 의존**: `WxCore` (유일한 Wx 의존; `IWxInteractable`·`WxGameplayTags`), 엔진: GameplayAbilities(GAS — GE/ASC), ModularGameplay(ControllerComponent 주입), StateTree(보상/리필 태스크), NetCore(FastArraySerializer), Niagara(픽업 이펙트, private).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Category + Fragment 배열 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` (+ `_Equippable/_Usable/_Charges/_Stackable/_Pickup/_Grade`) | 기능 축을 컴포지션하는 EditInline 인스턴스 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 개별 아이템의 수명·식별 단위(충전량 등 가변 상태 복제) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxInventoryManagerComponent` | PlayerController 부착, Add/Consume/Use/Refill/Equip 서버 권한 API | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | FastArray 인벤토리 슬롯 컨테이너 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관·복제, EquipEffect GE 관리, 외형 방송 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 지급 서버 권위 진입점(픽업 스폰/직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` / `FWxItemRewardEntry` | 보상 DataTable Row(최대 5항목, Soft 참조) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |
| `AWxItemPickup` | 픽업 액터(`IWxInteractable`), 상호작용 시 인벤토리 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxStateTreeTask_GiveRewards` / `_RefillItemCharges` | StateTree 태스크(보상 지급 / 충전 리필) | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h` |

## 확장 포인트 / 규약
- **새 아이템**: 코드 변경 없이 `UWxItemDefinition` 자산을 만들고 Category 지정 + 필요한 Fragment 를 `Fragments` 배열에 EditInline 인스턴스로 부착한다. 카테고리(무엇인가)와 Fragment(무엇을 할 수 있는가)는 직교 — 조합으로 행동을 차별화한다.
- **새 기능 축**: `UWxItemFragment` 를 상속하고 `OnInstanceCreated`(인스턴스 초기 상태 주입)를 오버라이드. 소비처는 `FindFragmentByClass<T>()` 로 조회한다.
- **충전형(에스트병)**: `_Charges` 부착 시 사용해도 인벤토리 스택은 차감되지 않고 인스턴스 충전량만 1 감소, `RefillItemCharges` 로 회복. `_Usable` 과 함께 있어야 사용이 성립.
- **보상**: `FWxRewardTableRow` DataTable 로우로 정의 → `UWxRewardLibrary::GrantReward` 또는 `FWxStateTreeTask_GiveRewards` 로 지급. Pickup Fragment 유무로 픽업 스폰/직접 지급이 갈린다.
- **리플리케이션 모델**: 인벤토리 목록은 `FWxInventoryList`(FastArraySerializer)로, 인스턴스 충전량은 `UWxItemInstance`(OnRep)로 복제된다. 서버·클라 양 경로가 매니저의 `Notify*FromList/FromSource` 진입점으로 수렴해 델리게이트(`OnInventoryStackChanged`/`SlotChanged`/`ChargeChanged`)를 발행한다. `AddItemDefinition`·`Consume*`·`Use*`·`Equip*` 은 서버 권한에서만 호출.
- **부착 방식**: 매니저 컴포넌트는 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 설정으로 PlayerController 에 붙는다. 관찰자(HUD 뷰모델)는 클래스 정적 `OnAnyInventoryReady` 로 준비 시점을 잡는다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment별 doc-comment가 카테고리/기능 축 분리와 각 축의 규약(장착·사용·충전·스택·픽업·등급)을 가장 압축해 설명한다. 도메인 모델의 출발점.
2. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 전 API·복제 구조·델리게이트가 모두 여기 있다. Add/Consume/Use/Refill/Equip 흐름을 잡는 중심.
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — FastArray 콜백과 서버 권한 변경 로직의 실제 구현.
4. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 보상 지급의 픽업/직접 지급 분기(월드·권위 진입점).

## 관련
- 상위: [[WxGame]] (Experience로 매니저 컴포넌트 주입, 무기 외형 반영), [[WxCore]] (`IWxInteractable`·`WxGameplayTags`)

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 22파일 — `/readme-writer`로 갱신*
