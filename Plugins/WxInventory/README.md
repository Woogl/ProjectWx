# WxInventory — 아이템 · 인벤토리 시스템

> 아이템의 정의·인스턴스·소유(인벤토리)와 획득/사용/보상 흐름을 책임진다. 플레이어 컨트롤러에 붙는 서버 권위 인벤토리를 중심으로, 데이터 자산(Definition)과 런타임 인스턴스(Instance)를 분리해 관리한다.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`) + Fragment 컴포지션과, 런타임 가변 상태를 담는 복제 인스턴스(`UWxItemInstance`)의 생성·소멸.
- 서버 권위 인벤토리 소유·수량·스택 관리와 FastArray 기반 클라이언트 동기화(`UWxInventoryComponent` / `FWxInventoryList`).
- 아이템 사용 흐름 조립: 사용 요청 → 어빌리티 발동 → AnimNotify 이벤트 → 소비/충전 차감 + GameplayEffect 적용.
- 보상 지급(`UWxRewardLibrary` + `FWxRewardTableRow`)과 픽업 액터 스폰/직접 지급, 충전형(에스트병) 리필.

**경계 (비담당)**
- 공용 GameplayTag(`Ability_UseItem`·`Event_UseItem`), `IWxInteractable` 계약, `AWxPlayerController` 정의는 [[WxCore]].
- 픽업을 잡아내는 상호작용 스캐너/입력 경로는 [[WxWorld]] (픽업은 WxCore 계약만으로 대상이 됨).
- 인벤토리 델리게이트를 구독하는 HUD/뷰모델 등 표시는 [[WxUI]].
- 사용 어빌리티 본체(몽타주 재생 등 GAS 발동측)와 GameplayEffect의 스탯 처리는 인벤토리 밖(소유 폰 ASC).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 컨트롤러 부착 권위 인벤토리. Add/Consume/Use/Refill의 공개 API 허브 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `FWxInventoryList` | FastArraySerializer 기반 엔트리 목록. 실제 추가/차감/복제 콜백 담당 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset) + Fragment 컬렉션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 개별 아이템의 복제 런타임 상태(충전량 등)·안정 식별자 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | 아이템 기능 축 컴포지션 베이스(Usable/Charges/Stackable/Pickup/Grade) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxRewardLibrary` | 보상 지급 서버 권위 진입점(픽업 스폰 또는 직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 지급용 픽업 액터. 상호작용 시 인벤토리에 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxItemUseComponent` | AnimNotify GameplayEvent를 받아 준비된 소비 아이템을 1회 사용 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxItemUseComponent.h` |

## 확장 포인트 / 규약
- 새 아이템 능력은 `UWxItemFragment`를 상속해 추가한다. Definition의 `Fragments`에 EditInline로 부착되고, 인스턴스 생성 직후 `OnInstanceCreated`로 초기 상태를 주입한다. 조회는 `FindFragmentByClass<T>()`.
- 아이템·보상은 데이터 주도: `UWxItemDefinition`은 `UPrimaryDataAsset`(NotBlueprintable — AssetManager 등록 일관성), 보상은 `FWxRewardTableRow` DataTable Row. `Item`은 `TSoftObjectPtr`라 지급 시점에 동기 로드된다.
- StateTree로 보상/리필을 건다: `FWxStateTreeTask_GiveRewards`, `FWxStateTreeTask_RefillItemCharges`. 둘 다 라이브 전이에서만 권위 측 실행(초기 진입/복원/레이트조인은 중복 방지로 스킵).
- 리플리케이션/권한: Add/Consume/Use/Refill은 모두 서버 권위에서만 호출. `FWxInventoryList`(FastArray)와 `UWxItemInstance`(ReplicatedUsing)로 클라 동기화되며, 클라 변경 통지는 `OnInventory*` 델리게이트로 전달된다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 공개 API·델리게이트·FastArray 구조가 모두 모여 모듈의 제어 흐름 지도 역할.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Definition의 데이터/행동이 Fragment로 어떻게 갈라지는지 한눈에.
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp` — 요청→어빌리티→AnimNotify(`Event_UseItem`)→차감/GE 적용의 파일 횡단 사용 흐름.

## 관련
- 상위: 인벤토리 컴포넌트는 [[WxCore]]의 `AWxPlayerController`에 기본 부착된다. 픽업 상호작용은 [[WxWorld]], 표시는 [[WxUI]], 사용 어빌리티/이펙트는 소유 폰의 GAS 경로와 맞물린다.

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 24파일 — `/readme-writer`로 갱신*
