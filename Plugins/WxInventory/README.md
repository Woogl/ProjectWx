# WxInventory — 아이템 및 인벤토리 시스템

> 아이템 정의(데이터 자산)와 런타임 인스턴스, 인벤토리 소유·소비, 장비, 보상 지급을 담당하는 Runtime 플러그인. 아이템의 "무엇을 할 수 있는가"는 Definition 에 EditInline 으로 붙는 Fragment 조합으로 컴포지션한다.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`)와 기능 축을 붙이는 Fragment 조합
- 런타임 아이템 인스턴스(`UWxItemInstance`)의 수명·식별·충전 상태
- PlayerController 부착 인벤토리(`UWxInventoryComponent`): FastArray 복제, 서버 권위 Add/Consume/Use/Refill
- 장비 상태 보관·복제와 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`)
- 보상 테이블 기반 지급(`UWxRewardLibrary`)과 픽업 액터 스폰·발사(`AWxItemPickup`)

**경계 (비담당)**
- 아이템 사용 GE 발동 어빌리티·ASC 는 [[WxCombat]]/GameplayAbilities 측 (본 모듈은 GE Spec 을 만들어 적용만)
- 무기 외형 반영(메시 스왑·소켓 재부착)은 게임 모듈이 `OnEquipVisualChanged` 방송을 받아 수행
- 픽업 상호작용 스캔·계약은 [[WxCore]]의 `IWxInteractable`, 스캐너는 [[WxWorld]]
- 인벤토리 컴포넌트 부착은 코드가 아니라 Experience 에셋 주입 설정

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset), Category + Fragment 컬렉션 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 아이템 기능 축을 컴포지션하는 EditInline 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 개별 아이템의 런타임 수명·식별·충전 상태 | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxInventoryComponent` | PlayerController 부착 인벤토리, 서버 권위 변경 + FastArray 복제 | `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 복제 + EquipEffect GE 관리, 외형은 델리게이트 방송 | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 로우 서버 권위 지급(픽업 스폰 또는 직접 지급) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(최대 5개 아이템·수량) | `Source/WxInventory/Public/Items/WxRewardTableRow.h` |
| `AWxItemPickup` | 상호작용형 아이템/재화 픽업 액터 | `Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 자산을 만들고 `Category` 지정 후 `Fragments` 에 필요한 Fragment 를 EditInline 으로 조합한다. 카테고리(무엇인가)와 Fragment(무엇을 할 수 있는가)는 직교한다.
- **새 기능 축**: `UWxItemFragment` 를 상속. 인스턴스 초기 상태 주입이 필요하면 `OnInstanceCreated` 를 오버라이드한다(예: `Charges` 가 MaxCharges 로 채움).
- **조회**: 정의·인스턴스 모두 템플릿 `FindFragmentByClass<T>()` 로 기능 유무를 판정한다.
- **데이터 주도 보상**: `FWxRewardTableRow` DataTable 을 만들고 `UWxRewardLibrary::GrantReward` 또는 StateTree Task(`FWxStateTreeTask_GiveRewards`)로 지급한다. `Item` 은 SoftPtr 라 지급 시점에 로드된다.
- **리플리케이션**: 인벤토리는 `FWxInventoryList`(FFastArraySerializer) 로 복제되고, 인스턴스는 `RegisterReplicatedInstance` 로 서브오브젝트 등록된다. 서버 권위에서만 Add/Consume/Use 를 호출할 것(주석에 "권한:" 표기).
- **충전형(에스트병)**: `Charges` Fragment 아이템은 인벤토리 스택이 아니라 인스턴스별 충전량으로 사용 가능 여부가 결정되며, 사용 시 스택은 유지하고 충전만 1 감소, `RefillItemCharges` 로 회복한다.
- **미구현 주의**: 장비 경로(`EquipItemByDef` → `EquipItemComponent::EquipItem`)는 배선만 있고 호출부가 0건이다(비-BlueprintCallable). `RemoveItemInstance` 도 호출부 0건.

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 인벤토리 변경 API 전체와 통지 델리게이트, FastArray 구조가 한 파일에 모여 있다.
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 표현할 수 있는 기능 축(Fragment) 전 목록. 시스템 능력 범위가 여기서 보인다.
3. `Source/WxInventory/Public/Items/WxItemDefinition.h` / `WxItemInstance.h` — Definition(정적) vs Instance(런타임) 의 역할 분리.

## 관련
- 상위: Experience 에셋의 주입 설정이 `UWxInventoryComponent` 를 PlayerController 에 부착. 보상 지급은 StateTree(`FWxStateTreeTask_GiveRewards`/`RefillItemCharges`)와 스포너(보물상자·적 드랍)에서 진입.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 22파일 — `/readme-writer`로 갱신*
