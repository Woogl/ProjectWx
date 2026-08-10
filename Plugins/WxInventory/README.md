# WxInventory — 인벤토리 시스템

> 아이템 정의(Fragment 컴포지션)와 런타임 인스턴스, PlayerController 부착 인벤토리, 장비/사용/충전, 픽업·보상 지급을 담당하는 도메인 플러그인.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 속성·행동 선언, 런타임 인스턴스(`UWxItemInstance`) 수명 관리.
- PlayerController 부착 인벤토리(`UWxInventoryManagerComponent`): FastArray 복제, 추가/소비/스택 머지, 사용(Usable)·충전(Charges) 처리.
- 장비 상태 보관·복제와 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`).
- 픽업 액터(`AWxItemPickup`) 상호작용 지급, 보상 테이블(`FWxRewardTableRow`) 서버 권위 지급(`UWxRewardLibrary`), StateTree 리필/보상 태스크.

**경계 (비담당)**
- 무기 외형 반영(메시 스왑/소켓 재부착)은 `OnEquipVisualChanged` 방송만 하고 실제 반영은 게임 모듈이 수행(무기 액터·ChildActor 접근 불가).
- UI 표현(인벤토리 탭/아이콘 렌더)은 WxUI/HUD 몫. 델리게이트만 노출.
- 아이템 사용 효과 자체는 GameplayEffect(GE)로 위임 — 본 모듈은 적용/차감만 조율.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존, `IWxInteractable`·`WxGameplayTags` 소비). 엔진: GameplayAbilities(ASC/GE), ModularGameplay(ControllerComponent), StateTree, NetCore(FastArray), Niagara(픽업 이펙트).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | PlayerController 부착 인벤토리. 추가/소비/사용/리필의 서버 권위 진입점, 슬롯/합계/충전 델리게이트 발행 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Fragment 컬렉션 + 카테고리 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | Fragment 베이스. Equippable/Usable/Charges/Stackable/Pickup/Grade 파생으로 기능 축 컴포지션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 런타임 인스턴스. 슬롯 델리게이트의 안정 식별자, 충전량 복제, GE SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장비 상태 보관·복제 + EquipEffect GE 라이프사이클(현재 트리거 미배선) | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 테이블 서버 권위 지급(픽업 스폰 or 인벤토리 직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 월드 픽업 액터. `IWxInteractable`로 상호작용 시 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(최대 5항목, 빈 슬롯 무시) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 데이터 자산을 만들고 `Fragments`에 Fragment 인스턴스를 부착(카테고리는 `Category`, 기능은 Fragment). 새 기능 축은 `UWxItemFragment` 파생 + `OnInstanceCreated`로 인스턴스 초기 상태 주입.
- **Fragment 조합 규약**: Stackable 부재 시 슬롯당 1개. Charges는 Usable과 직교 — 충전형 소비 아이템은 Usable과 함께 부착해야 사용이 성립.
- **보상 추가**: `FWxRewardTableRow` DataTable에 Row 추가(RowName 예: `Reward_Chest_Rare`). Pickup Fragment 있으면 월드 드랍, 없으면 인벤토리 직접 지급.
- **권한/복제 모델**: Add/Consume/Use/Equip은 서버 권위 전용. `FWxInventoryList`(FastArraySerializer)로 클라 동기화, 인스턴스 충전량은 `OnRep_CurrentCharges`. 서버 변경 경로와 클라 복제 콜백이 모두 `Notify*FromList/FromSource` 통지 진입점으로 수렴.
- **부착 방식**: 인벤토리 컴포넌트는 코드가 아닌 Experience 에셋 주입으로 PlayerController에 붙는다. 관찰자는 `static OnAnyInventoryReady`로 준비 시점을 받고 소유 액터로 자기 것인지 판별.
- **미배선 주의**: `EquipItemByDef`/`RemoveItemInstance` 호출부 0건 — 장비 경로는 배선만 있고 트리거 없음.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 전체 API·복제·통지 흐름의 중심.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템 행동이 어떻게 데이터로 조립되는지(Fragment 목록).
3. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` — 정의/인스턴스 분리와 Fragment 조회 진입.
4. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 픽업 vs 직접 지급 분기의 서버 진입점.

## 관련
- 상위: [[WxCore]] (foundation — `IWxInteractable`, `WxGameplayTags`)

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 22파일 — `/readme-writer`로 갱신*
