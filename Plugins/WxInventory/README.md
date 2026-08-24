# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(Definition)와 런타임 인스턴스, 플레이어별 인벤토리 보관·복제, 사용/장착/충전, 그리고 보상 지급·드랍 픽업을 담당하는 도메인 플러그인이다.

## 책임
**담당**
- 아이템 데이터 모델: `UWxItemDefinition`(정적 정의) + `UWxItemFragment` 조합(Equippable/Usable/Charges/Stackable/Pickup/Grade)으로 행동을 컴포지션.
- 런타임 인스턴스(`UWxItemInstance`)의 생성·소멸·복제와 슬롯 단위 안정 식별자 제공.
- 인벤토리 보관·복제: `FWxInventoryList`(FastArraySerializer) 기반 서버 권위 Add/Consume, 클라 델타 동기화, 스택 머지/분할.
- 아이템 사용(Usable GE 적용 + 스택 차감)과 다크소울식 충전형(Charges) 소모/리필.
- 장비 상태 보관과 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`) — 외형 반영은 방송만.
- 보상 지급(`UWxRewardLibrary`)과 드랍 픽업 액터(`AWxItemPickup`), StateTree 진입 지급/리필 태스크.

**경계 (비담당)**
- 무기 메시 스왑·소켓 재부착 등 외형 반영: `OnEquipVisualChanged`로 방송만 하고 게임 모듈(캐릭터/ChildActor)이 실제 반영.
- 인벤토리 UI 표시: 델리게이트만 노출하며 뷰모델/위젯은 [[WxUI]].
- 상호작용 스캔·프롬프트 표시 메커니즘: 픽업은 `IWxInteractable`(WxCore 계약)만 구현하고 스캐너는 [[WxWorld]].
- 아이템 사용의 실제 게임플레이 효과(GAS 어빌리티/이펙트 정의): GE는 데이터로 참조만, 발동 경로는 소유 폰 어빌리티([[WxCombat]]).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | PlayerController 부착, 모든 Add/Consume/Use/Equip 진입점 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` | FastArray 복제 컨테이너, 권위 측 엔트리 조작의 실제 구현 | 같은 헤더 상단 |
| `UWxItemDefinition` | 아이템 정적 정의 데이터 자산(카테고리 + Fragment 배열) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 개별 아이템의 런타임 수명/식별 단위, 충전량 보관·복제 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 복제 + EquipEffect GE 관리, 외형 변경 방송 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 테이블 로우 서버 권위 지급(픽업 스폰 또는 직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 드랍/상호작용 픽업 액터, 상호작용 시 인벤토리 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- **새 아이템 행동 추가**: `UWxItemFragment` 파생 클래스를 만들어 Definition 의 `Fragments`에 EditInline 인스턴스로 부착한다. 인스턴스 초기 상태가 필요하면 `OnInstanceCreated`를 오버라이드(예: `UWxItemFragment_Charges`가 충전량 시드). Fragment 는 정의당 단일 객체로 모든 인스턴스가 공유하므로 가변 상태는 `UWxItemInstance`에 둔다.
- **카테고리 vs 기능**: 분류는 `EWxItemCategory`가, "무엇을 할 수 있는가"는 Fragment 조합이 표현한다(직교). Fragment 조회는 `FindFragmentByClass<T>()` 템플릿(Definition/Instance 양쪽).
- **리플리케이션 모델**: 서버 권위에서만 Add/Consume/Use/Equip 을 호출한다. `FWxInventoryList`가 FastArraySerializer 로 델타 복제하고, 변경은 서버 경로·클라 복제 콜백 모두 `Notify*FromList/FromSource` 진입점으로 수렴해 `OnInventoryStackChanged`/`OnInventorySlotChanged`/`OnInventoryChargeChanged`를 발행한다.
- **인벤토리 획득 시점**: 관찰자(HUD 등)가 인벤토리보다 먼저 생길 수 있어 준비 통지는 인스턴스가 아닌 클래스 static `UWxInventoryManagerComponent::OnAnyInventoryReady`로 방송한다. 조회는 `FindInventory(Actor)`(폰→소유 컨트롤러 경유).
- **컴포넌트 부착**: 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 설정으로 PlayerController 에 붙인다(등록 안 하면 인벤토리가 조용히 없음).
- **StateTree 통합**: `FWxStateTreeTask_GiveRewards`(보상 지급)·`FWxStateTreeTask_RefillItemCharges`(체크포인트 리필)는 라이브 전이 진입 시 권위 측에서만 실행되며, 초기 진입/복원/레이트조인에서는 중복 방지를 위해 노옵.
- **미배선 경로 주의**: 장비 경로(`EquipItemByDef`→`EquipItem`)와 `RemoveItemInstance`는 현재 호출부가 0건이다(BlueprintCallable 도 아님). 배선만 있고 트리거가 없는 상태 — 상세는 각 헤더 doc-comment 참조.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 모듈 전체 진입점. 인벤토리 API 전체 표면과 `FWxInventoryList`/엔트리 구조, 델리게이트 3종이 한 파일에 모여 있다.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 데이터 모델의 핵심. 6종 Fragment 가 각각 어떤 기능 축을 담당하는지 보면 아이템 행동 전반이 잡힌다.
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — 스택 머지/분할, 원자적 소비, 통지 수렴의 실제 구현.
4. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 보상 지급이 픽업 스폰과 직접 지급으로 갈리는 규약.

## 관련
- 상위: Experience 에셋(GameMode)이 매니저 컴포넌트를 주입하고, StateTree(레벨/게임플로우)가 지급·리필 태스크를 구동한다. 뷰는 [[WxUI]], 픽업 상호작용은 [[WxWorld]], 사용 효과·장비 외형은 [[WxCombat]]/게임 모듈과 함께 본다.

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 22파일 — `/readme-writer`로 갱신*
