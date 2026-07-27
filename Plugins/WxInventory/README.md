# WxInventory — 아이템/인벤토리 시스템

> 아이템 정적 정의(Fragment 컴포지션)와 런타임 인스턴스, 서버 권위 인벤토리/장비 관리, 보상 지급·픽업 드랍을 담당하는 도메인 플러그인. FastArray 로 인벤토리를 복제하고, GAS(GameplayEffect)로 사용/장착 효과를 반영한다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션(Equippable/Usable/Charges/Stackable/Pickup/Grade)으로 데이터 주도 아이템 선언
- 런타임 인스턴스(`UWxItemInstance`) 생성·소멸·복제, 슬롯/충전량 상태 관리
- 인벤토리 매니저(`UWxInventoryManagerComponent`): 추가/차감/스택 머지·분할, 사용(GE 적용), 충전형(에스트병) 사용·리필, FastArray 복제
- 장비 컴포넌트(`UWxEquipmentComponent`): 장착 ItemDef 보관·복제, EquipEffect GE 라이프사이클
- 보상 지급(`UWxRewardLibrary::GrantReward`) 및 월드 픽업(`AWxItemPickup`) 드랍/획득

**경계 (비담당)**
- 무기 외형 반영(메시 스왑/소켓 부착): 장비 컴포넌트는 `OnEquipVisualChanged` 로 메시/소켓만 방송하고, 실제 반영은 게임 모듈(캐릭터)이 수행
- 상호작용 스캔/프롬프트 배선: 픽업은 `IWxInteractable`(WxCore 계약)만 구현하며 스캐너는 [[WxWorld]] 소관
- UI 출력(인벤토리 탭/아이콘 렌더): [[WxUI]]

## 의존성
- **주요 의존**: `WxCore`(`WxInteractable` 계약 인터페이스), `GameplayAbilities`/`GameplayTags`(사용·장착 GE 적용, ASC), `StateTreeModule`(GrantReward·RefillItemCharges Task), `NetCore`(FastArray/SubObject 복제), `Niagara`(픽업 이펙트, private)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 검증 — 없음 ✅ (WxCore 만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 인벤토리 소유·복제 허브. Add/Consume/Use/Equip/Refill 진입점. `FindInventory(Actor)` 로 조회 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(`UPrimaryDataAsset`). Fragment 컬렉션 + Category 선언 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 아이템 행동 컴포지션 베이스. 파생: Equippable/Usable/Charges/Stackable/Pickup/Grade | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 런타임 인스턴스(수명·식별·충전량). GAS SourceObject | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 복제 + EquipEffect GE 관리, 외형 변경 방송 | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `AWxItemPickup` | 월드 드랍 픽업 액터(`IWxInteractable`). 획득 시 인벤토리 지급 후 파괴 | `Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxRewardLibrary` | 보상 지급 서버 권위 진입점(픽업 스폰 vs 직접 지급 분기) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(아이템,수량 Pair 최대 5). 지연 로드 | `Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`(`EWxItemCategory`) 지정 + `Fragments` 배열에 필요한 Fragment 를 EditInline 으로 조합한다. 행동은 Fragment 조합으로 결정 — 스택 가능하려면 `Stackable`(MaxStack), 사용 효과는 `Usable`(GE), 충전형 소비는 `Charges`+`Usable`, 장비는 `Equippable`(메시/소켓/EquipEffects), 월드 드랍은 `Pickup`, 등급/색은 `Grade`.
- **새 Fragment**: `UWxItemFragment` 를 상속하고 필요 시 `OnInstanceCreated(Instance)` 오버라이드로 인스턴스 초기 상태를 주입한다(예: Charges 가 MaxCharges 로 시드).
- **보상**: `FWxRewardTableRow` DataTable 을 채우고, StateTree 로 지급하려면 `FWxStateTreeTask_GrantReward`(RewardRow/SpawnOffset/LaunchVelocity), 체크포인트 리필은 `FWxStateTreeTask_RefillItemCharges` Task 를 배치한다. 둘 다 라이브 전이·권위 측에서만 1회 발동.
- **권위 규약**: Add/Consume/Use/Equip/Refill 은 서버 권한에서만 호출하고 클라이언트는 FastArray/SubObject 복제로 추종한다. 픽업 지급 데이터는 SpawnActorDeferred → `SetItemDef` → FinishSpawning 흐름으로 주입.

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 데이터 모델(Entry/List FastArray)과 모든 변경/통지 진입점, 스택 머지·차감·충전 정책이 한곳에 서술됨
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 6종이 아이템 행동을 어떻게 컴포지션하는지, 카테고리와 기능 축의 분리 원칙
3. `Source/WxInventory/Public/WxRewardLibrary.h` — 보상 지급이 픽업 드랍/직접 지급으로 갈리는 서버 권위 흐름

## 관련
- 상위: [[WxGame]] (인벤토리/장비 컴포넌트 부착·외형 반영), [[WxWorld]] (픽업 상호작용 스캔), [[WxUI]] (인벤토리 표시)
- 기반: [[WxCore]] (`WxInteractable` 계약)

---
*문서 기준 커밋 `21e2e76` · 생성일 2026-07-27 · 소스 21파일 — `/readme-writer`로 갱신*
