# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정의(데이터 자산)와 런타임 인스턴스, PlayerController 부착 인벤토리, 장비·사용·충전·보상 지급 경로를 책임진다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 아이템의 데이터·행동을 기술
- PlayerController 부착 인벤토리(`UWxInventoryComponent`): FastArray 기반 서버 권한 추가/소비/스택 머지, 슬롯·합계·충전 변경 통지
- 아이템 인스턴스(`UWxItemInstance`) 생성·소멸·복제 및 충전량(에스트병식) 관리
- 보상 테이블 지급(`UWxRewardLibrary::GrantReward`) — 픽업 스폰 또는 인벤토리 직접 지급
- 장비 GE 라이프사이클과 외형 변경 방송(`UWxEquipmentComponent`)
- StateTree 태스크 2종(보상 지급, 충전 리필)

**경계 (비담당)**
- 상호작용 스캔·`IWxInteractable` 계약 정의는 [[WxCore]] (픽업이 이 계약만으로 스캐너에 잡힌다)
- 무기 액터·메시 반영(ChildActor 소유)은 게임 모듈 — 장비는 메시/소켓을 방송만 하고 반영은 위임
- UI 표시는 [[WxUI]] (인벤토리는 델리게이트만 발행)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 인벤토리 진입점 — 추가/소비/사용/장착 API, 변경 델리게이트 | `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset) + Fragments 컬렉션 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 개별 아이템의 수명·식별·충전량 단위(복제) | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxRewardLibrary` | 보상 지급 서버 권위 진입점(무상태 라이브러리) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` | 보상 DataTable 로우(최대 5항목, 아이템은 소프트 참조) | `Source/WxInventory/Public/Items/WxRewardTableRow.h` |
| `AWxItemPickup` | 지급용 픽업 액터(Abstract) — 상호작용 시 인벤토리에 지급 후 파괴 | `Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxEquipmentComponent` | 장비 GE 라이프사이클 + 외형 변경 방송 | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 데이터 자산을 만들고 `Category` 지정 후 `Fragments`에 필요한 EditInline Fragment 를 조합한다. Category(무엇인가)와 Fragment(무엇을 할 수 있나)는 직교한다.
- **새 기능 축**: `UWxItemFragment` 를 상속. 인스턴스 초기 상태가 필요하면 `OnInstanceCreated` 오버라이드(예: `UWxItemFragment_Charges`). `FindFragmentByClass<T>()` 로 조회.
- **스택**: `Stackable` Fragment 있으면 `MaxStack` 한도까지 슬롯 머지, 없으면 슬롯당 1개 강제.
- **충전형(에스트병)**: `Charges` Fragment. 사용 시 인벤토리 스택이 아니라 인스턴스 충전량이 1 감소하고, 리필로 `MaxCharges` 회복. `Usable` 없이 단독이면 사용 자체가 성립 안 함.
- **보상**: `FWxRewardTableRow` DataTable + `UWxRewardLibrary::GrantReward`. `Pickup` Fragment 있으면 픽업 스폰, 없으면(재화 등) `DirectGrantTarget` 인벤토리에 직접 지급.
- **리플리케이션/권한**: Add/Consume/Use/Equip 는 모두 서버 권한 전용. `FWxInventoryList`(FastArraySerializer)가 클라 동기화. 부착은 코드가 아니라 Experience 에셋 주입으로 하며(PlayerController 는 본 컴포넌트를 모름), `UWxInventoryComponent::OnAnyInventoryReady`(클래스 차원 static)로 뷰모델이 준비 시점을 관찰한다.
- **미배선 경로**: `EquipItemByDef`/`RemoveItemInstance` 는 현재 호출부 0건(장비 경로는 배선만 존재).

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 전체 API 표면과 권한/복제/통지 모델이 여기 다 있다
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 표현할 수 있는 기능 축 전부(스택/사용/충전/장비/픽업/등급)
3. `Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp` — 스택 머지·원자적 소비·사용/차감 순서의 실제 흐름

## 관련
- 상위: Experience 에셋이 인벤토리 컴포넌트를 PlayerController 에 주입, StateTree(체크포인트 리필·보상 지급)와 상호작용([[WxCore]] `IWxInteractable`)이 소비처. UI 표시는 [[WxUI]].

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 22파일 — `/readme-writer`로 갱신*
