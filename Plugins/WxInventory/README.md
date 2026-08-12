# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(Fragment 컴포지션)와 런타임 인스턴스, PlayerController 부착 인벤토리, 보상 지급·픽업·장비 배선을 담당하는 도메인 플러그인. 서버 권위 + FastArray 복제 모델을 따른다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 속성·행동을 데이터 주도로 선언
- 런타임 아이템 인스턴스(`UWxItemInstance`)의 생성·소멸·복제 및 충전(charge) 상태 관리
- PlayerController 부착 인벤토리(`UWxInventoryManagerComponent`): 추가/소비/사용/리필과 스택·슬롯·충전 변경 통지
- 보상 데이터(`FWxRewardTableRow`) 지급과 월드 픽업(`AWxItemPickup`) 스폰/발사/획득
- 장착 상태 보관과 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`)

**경계 (비담당)**
- 무기 외형 반영(메시 스왑/소켓 재부착) — `OnEquipVisualChanged`로 방송만 하고 게임 모듈이 반영
- 상호작용 스캔·프롬프트 표시 — [[WxWorld]]; 픽업은 [[WxCore]]의 `IWxInteractable` 계약으로만 응답
- 아이템 사용 어빌리티(GAS) 실행 판정 — 소유 폰의 UseItem 어빌리티 경로에 위임
- 인벤토리 UI 표현 — [[WxUI]]

## 의존성
- **주요 의존**: [[WxCore]] (유일한 Wx 의존, `IWxInteractable` 등), GameplayAbilities(GE 적용), ModularGameplay(`UControllerComponent`), StateTree(보상/리필 Task), NetCore(FastArray), Niagara(픽업 이펙트, private)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | PlayerController 부착 인벤토리 허브. 추가·소비·사용·리필·장착 요청의 서버 권위 진입점 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Category + Fragment 컬렉션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 아이템 한 자루의 런타임 인스턴스. 충전량 복제, 슬롯 델리게이트의 안정 식별자 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | Fragment 베이스(Equippable/Usable/Charges/Stackable/Pickup/Grade). 행동을 컴포지션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관·복제 + EquipEffect GE 라이프사이클. 외형은 방송만 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 지급의 서버 권위 진입점(무상태 BP 라이브러리). 픽업 드랍/직접 지급 분기 | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 월드 픽업 액터. `IWxInteractable`로 자기 메시를 답하고 획득 시 인벤토리에 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(최대 5항목). `FWxItemRewardEntry`가 아이템·수량을 담음 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템 추가**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`를 정한 뒤, 필요한 Fragment를 `Fragments`에 EditInline 인스턴스로 조합한다. 코드 수정 불필요.
- **새 아이템 행동**: `UWxItemFragment`를 상속해 데이터/훅을 추가한다. `OnInstanceCreated`로 인스턴스 초기 상태를 주입한다(예: Charges가 초기 충전량 채움).
- **Fragment 기능 축**: Stackable(슬롯 머지), Usable(사용 GE), Charges(에스트병식 인스턴스 충전 — Usable과 함께 부착), Equippable(메시/소켓/EquipEffects), Pickup(픽업 액터 클래스·외형), Grade(등급·색상). Category 축과 직교.
- **리플리케이션 모델**: 인벤토리는 `FWxInventoryList`(FastArraySerializer)로 복제. `Add*`/`Consume*`/`Use*`/`Refill*`은 서버 권위에서만 호출. 서버 변경 경로와 클라 복제 콜백이 `Notify*FromList/FromSource` 진입점으로 수렴해 스택/슬롯/충전 델리게이트를 발행한다.
- **보상 지급**: `FWxRewardTableRow`를 DataTable로 저작하고 `UWxRewardLibrary::GrantReward` 또는 StateTree Task(`WxStateTreeTask_GiveRewards`)로 트리거. Pickup Fragment 유무로 월드 드랍/직접 지급이 갈린다.
- **미배선 주의**: 장비 경로(`EquipItemByDef`→`UWxEquipmentComponent::EquipItem`)는 배선만 있고 호출 트리거가 없어 현재 동작하지 않는다. 사용하려면 UI 슬롯→어빌리티/서버 RPC→`EquipItemByDef` 경로를 닫아야 한다(헤더 주석 참조).

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리의 모든 변경 경로와 복제 모델(`FWxInventoryList`)이 여기 모인다. 시스템의 중심.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 "무엇을 할 수 있는가"를 정하는 컴포지션 축. 정의/인스턴스 동작을 이해하는 열쇠.
3. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 획득이 어떻게 인벤토리로 흘러드는지(픽업 vs 직접 지급)의 진입점.

## 관련
- 기반: [[WxCore]] (`IWxInteractable` 계약)
- 소비처: [[WxUI]] (인벤토리 표현), [[WxWorld]] (상호작용 스캔), [[WxCombat]]/게임 모듈 (사용·장비 어빌리티)

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 22파일 — `/readme-writer`로 갱신*
