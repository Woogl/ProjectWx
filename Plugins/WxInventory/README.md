# WxInventory — 아이템 · 인벤토리 시스템

> 아이템의 정적 정의(Definition)와 런타임 인스턴스, 소유 인벤토리의 생성·소비·복제, 그리고 보상 지급·월드 드랍·장비/충전 아이템을 담당한다. 인벤토리는 PlayerController 에 붙어 서버 권한에서만 변경되고 FastArray 로 클라에 동기화된다.

## 책임
**담당**
- 아이템 데이터 모델: `UWxItemDefinition`(정적) + `UWxItemInstance`(런타임 수명·충전량) + Fragment 컴포지션
- 소유 인벤토리 관리(`UWxInventoryManagerComponent`): 추가/소비/조회, 정의·슬롯·충전 단위 변경 델리게이트
- 보상 지급 파이프라인: `FWxRewardTableRow`(DataTable) → `UWxRewardLibrary::GrantReward` → 월드 픽업 드랍 또는 인벤토리 직접 지급
- 사용/충전형(에스트병) 소비: `UseItemByDef`, `RefillItemCharges` 및 GameplayEffect 적용
- 장비 상태 보관·복제와 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`) — 단, 트리거 미배선(아래 경계 참조)
- StateTree 훅(`FWxStateTreeTask_GiveRewards`, `FWxStateTreeTask_RefillItemCharges`)

**경계 (비담당)**
- 무기 외형 반영(메시 스왑/소켓 재부착): `UWxEquipmentComponent`는 메시/소켓을 `OnEquipVisualChanged`로 방송만 하고, 반영은 무기 액터·캐릭터를 소유한 게임 모듈([[WxGame]])이 수행
- 상호작용 스캔·프롬프트 표시 로직: 픽업은 `IWxInteractable`([[WxCore]] 계약)만 구현하며 스캐너·UI는 [[WxWorld]]·[[WxUI]] 소관
- 인벤토리 컴포넌트 부착: 코드가 아니라 Experience 에셋의 컴포넌트 주입([[WxGame]]/GameFeature)이 담당
- UseItem 어빌리티의 사용 가능 판정·발동: 소유 폰의 GAS 어빌리티([[WxCombat]] 등)가 수행, 인벤토리는 진입점만 제공

## 의존성
- **주요 의존**: [[WxCore]](계약 인터페이스 `IWxInteractable`, `WxGameplayTags`, `WxCollisionChannels`), GameplayAbilities(GAS/GE), ModularGameplay(`UControllerComponent`), StateTree, NetCore(FastArray), Niagara(픽업 이펙트, private)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (코드가 참조하는 Wx 헤더는 모두 WxCore 소속)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 인벤토리 진입점. PlayerController 부착, 추가/소비/사용/조회의 서버 권한 API 집합 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | 위 컴포넌트의 복제 저장소(FastArraySerializer). 슬롯 추가·머지·차감의 실무 로직 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템의 정적 정의(PrimaryDataAsset). Category + Fragment 컬렉션 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 아이템 기능 축의 컴포지션 베이스. Equippable/Usable/Charges/Stackable/Pickup/Grade 파생 | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 아이템 한 자루의 런타임 인스턴스. 충전량 보유, 슬롯 델리게이트의 안정 식별자 | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxRewardLibrary` | 보상 지급의 서버 권위 진입점. 픽업 드랍 vs 인벤토리 직접 지급 분기 | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 월드 드랍 픽업 액터(`IWxInteractable`). 상호작용 시 인벤토리에 지급 후 파괴 | `Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관/복제 + EquipEffect GE 관리(외형은 방송만) | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |

## 확장 포인트 / 규약
- 새 아이템 = `UWxItemDefinition` 데이터 자산 생성 후 `Category` 지정 + `Fragments` 배열에 필요한 Fragment 를 EditInline 조합. 카테고리(`EWxItemCategory`)는 UI/분류 축, Fragment 는 "무엇을 할 수 있는가" 기능 축으로 직교한다.
- 새 기능 축 추가 = `UWxItemFragment` 파생 클래스 작성. 인스턴스 초기 상태 주입이 필요하면 `OnInstanceCreated` 오버라이드(예: `UWxItemFragment_Charges`가 MaxCharges 로 시드).
- 스택/충전 규약: `Stackable` Fragment 있으면 슬롯당 MaxStack 까지 머지, 없으면 슬롯당 1개. `Charges` Fragment 는 사용 시 인벤토리 스택이 아니라 인스턴스 충전량만 감소(소진돼도 아이템은 남음), `Usable`과 함께 부착해야 사용이 성립.
- 데이터 주도 보상: `FWxRewardTableRow`(DataTable Row, 항목 최대 5개) → `GrantReward`. `Pickup` Fragment 유무로 월드 드랍/직접 지급이 갈린다. StateTree 에서는 `FWxStateTreeTask_GiveRewards`가 Row 를 물려 트리거.

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 모듈의 얼굴. `FWxInventoryList`의 서버 변경 경로와 클라 복제 콜백이 어떻게 통지 델리게이트로 수렴하는지 헤더 주석에 서술되어 있다.
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템 행동이 Fragment 컴포지션으로 어떻게 갈라지는지. 아이템을 이해하는 핵심.
3. `Source/WxInventory/Public/WxRewardLibrary.h` + `Private/WxRewardLibrary.cpp` — 지급 파이프라인의 분기 규칙(픽업 드랍 vs 직접 지급).

## 관련
- 상위: Experience/GameFeature([[WxGame]])가 컴포넌트를 주입하고, 무기 외형은 [[WxGame]]이 `OnEquipVisualChanged`를 구독해 반영한다. 픽업 상호작용은 [[WxWorld]] 스캐너·[[WxUI]] 프롬프트와, 아이템 사용 GE 는 [[WxCombat]] GAS 와 맞물린다.

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 22파일 — `/readme-writer`로 갱신*
