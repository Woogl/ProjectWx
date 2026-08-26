# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(Definition)와 런타임 인스턴스, 플레이어별 인벤토리 보관·복제, 사용/장착/충전, 보상 지급·드랍 픽업을 담당하는 도메인 플러그인이다.

## 책임
**담당**
- 아이템 데이터 모델: `UWxItemDefinition`(정적 정의) + `UWxItemFragment` 조합(Equippable/Usable/Charges/Stackable/Pickup/Grade)으로 기능 축을 컴포지션.
- 런타임 인스턴스(`UWxItemInstance`)의 생성·소멸·복제와 슬롯 단위 안정 식별자 제공.
- 인벤토리 보관·복제: `FWxInventoryList`(FastArraySerializer) 기반 서버 권위 Add/Consume, 클라 델타 동기화, 스택 머지/분할.
- 아이템 사용(Usable GE 적용 + 스택 차감)과 다크소울식 충전형(Charges) 소모/리필.
- 장비 상태 보관과 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`) — 외형 반영은 방송만.
- 보상 지급(`UWxRewardLibrary`)과 드랍 픽업 액터(`AWxItemPickup`), StateTree 진입 지급/리필 태스크.

**경계 (비담당)**
- 무기 메시 스왑·소켓 재부착 등 외형 반영: `OnEquipVisualChanged`로 방송만 하고 게임 모듈(캐릭터/ChildActor)이 실제 반영.
- 인벤토리 UI 표시: 델리게이트만 노출하며 뷰모델/위젯은 [[WxUI]].
- 상호작용 스캔·프롬프트 표시: 픽업은 `IWxInteractable`(WxCore 계약)만 구현하고 스캐너는 [[WxWorld]].
- 아이템 사용의 실제 게임플레이 효과 발동 경로: GE는 데이터로 참조만, 발동은 소유 폰 어빌리티([[WxCombat]]).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | PlayerController 부착, 모든 Add/Consume/Use/Equip 진입점 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` | FastArray 복제 컨테이너, 권위 측 엔트리 조작(Add/Remove/Consume)의 실제 구현 | 같은 헤더 상단 |
| `UWxItemDefinition` | 아이템 정적 정의 데이터 자산(카테고리 + Fragment 배열) | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 개별 아이템의 런타임 수명/식별 단위, 충전량 보관·복제 | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 복제 + EquipEffect GE 관리, 외형 변경 방송 | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 테이블 로우 서버 권위 지급(픽업 스폰 또는 직접 지급) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 드랍/상호작용 픽업 액터, 상호작용 시 인벤토리 지급 후 파괴 | `Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- 새 아이템 행동은 `UWxItemFragment` 파생 클래스로 추가하고, `UWxItemDefinition::Fragments`에 EditInline 인스턴스로 부착한다(카테고리는 `EWxItemCategory`, 기능은 Fragment). 조회는 `FindFragmentByClass<T>()` 템플릿.
- 데이터 주도: 아이템은 `UWxItemDefinition`(PrimaryDataAsset), 보상은 `FWxRewardTableRow`(DataTable Row, 항목별 `FWxItemRewardEntry`). Item 은 `TSoftObjectPtr`라 지급 시점에 로드된다.
- 리플리케이션: 인벤토리는 `FWxInventoryList` FastArray 로 델타 동기화되며, Add/Consume/Use/Equip/Refill 은 서버 권위 경로다. 서버 변경과 클라 복제 콜백이 모두 `Notify*FromList`/`Notify*FromSource` 진입점으로 수렴해 `OnInventoryStackChanged`/`OnInventorySlotChanged`/`OnInventoryChargeChanged` 델리게이트를 발행한다.
- 부착은 코드가 아니라 Experience 에셋의 컴포넌트 주입으로 하며, 관찰자는 `UWxInventoryManagerComponent::OnAnyInventoryReady`(클래스 정적)로 준비 시점을 받는다.
- 미배선 주의: 장비 경로(`EquipItemByDef` → `UWxEquipmentComponent::EquipItem`)는 호출부가 0건이라 현재 트리거되지 않는다(헤더 주석 참조).

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 전 기능의 공개 진입점과 FastArray 컨테이너·델리게이트 계약이 한곳에 있다.
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템 행동을 이루는 6종 Fragment 를 보면 데이터 모델 전체가 잡힌다.
3. `Source/WxInventory/Public/WxRewardLibrary.h` — 픽업 스폰 vs 직접 지급으로 갈리는 보상 지급 경로.

## 관련
- 상위: 사용 발동은 [[WxCombat]], UI 표시는 [[WxUI]], 픽업 상호작용 스캔은 [[WxWorld]].

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 22파일 — `/readme-writer`로 갱신*
