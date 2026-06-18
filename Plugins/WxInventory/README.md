# WxInventory — 아이템/인벤토리 시스템

> 아이템의 정적 정의(Fragment 컴포지션)와 런타임 인스턴스, 인벤토리 보관·사용·장착·보상 드랍을 책임지는 도메인 플러그인. 인벤토리는 서버 권한에서 변경되고 FastArray 로 클라이언트에 복제된다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`)와 Fragment 컴포지션으로 속성·행동 선언
- 런타임 인스턴스(`UWxItemInstance`)의 생성·소멸·복제, 충전(charge) 상태 관리
- 인벤토리 보관/머지/차감/사용(`UWxInventoryManagerComponent`)과 슬롯·정의·충전 단위 변경 통지
- 장비 GE 라이프사이클 관리 및 장착 외형 변경 방송(`UWxEquipmentComponent`)
- 보상 드랍: 픽업 액터 스폰·발사 또는 인벤토리 직접 지급(`UWxRewardComponent`, `AWxItemPickup`)

**경계 (비담당)**
- 무기 메시 스왑/소켓 재부착 등 실제 외형 반영 — `OnEquipVisualChanged` 방송만 하고 게임 측이 반영
- 상호작용 트리거링 — `IWxInteractionSource`(WxCore) 인터페이스를 통해 외부 구현에 위임
- 인벤토리 UI 출력 — [[WxUI]]
- GE 정의/스탯 적용 세부 — GameplayAbilities(ASC) 위임

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractionSource`), GameplayAbilities/GameplayTags(GE 적용·ASC), NetCore(FastArray 복제), Niagara(픽업 이펙트), DeveloperSettings
- 규칙: 「WxCore 외 Wx 참조」 — 없음 ✅ (상호작용은 WxCore 의 `WxInteractionSource.h` 경유)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 인벤토리 보관·머지·차감·사용·장착 요청의 중심 진입점. PlayerController 에 부착 | `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Fragment 컬렉션으로 행동 선언 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | 아이템 기능 축의 베이스. 파생: Equippable/Usable/Charges/Stackable/Pickup/Grade | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 아이템 한 자루의 런타임 인스턴스. 슬롯 델리게이트 안정 식별자·충전 상태 보유 | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관·복제와 EquipEffect GE 라이프사이클. 외형은 방송만 | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardComponent` | 보상 드랍 진입점. 픽업 스폰/발사 또는 직접 지급 | `Source/WxInventory/Public/Inventory/WxRewardComponent.h` |
| `AWxItemPickup` | 월드에 드랍되어 상호작용 시 인벤토리에 지급되는 픽업 액터(Abstract) | `Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(아이템·수량 Pair 최대 5) | `Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템 행동 추가**: `UWxItemFragment` 파생 클래스를 만들고 `UWxItemDefinition::Fragments` 에 EditInline 으로 부착. 인스턴스 초기 상태 주입이 필요하면 `OnInstanceCreated` override
- **데이터 주도 설정**: 아이템은 `UWxItemDefinition` 데이터 자산, 보상은 `FWxRewardTableRow` DataTable. 자산 참조는 `TSoftObjectPtr`/`TSoftClassPtr` 로 지연 로드
- **카테고리 vs 기능**: `EWxItemCategory`(정의 필드)가 UI/기능 1차 분기 축, Fragment 는 직교한 기능 축. 충전형 소비 아이템은 Charges + Usable 동반 부착
- **권한 모델**: Add/Consume/Use/Equip/Refill 은 서버 권한 전용. 인벤토리는 `FWxInventoryList`(FastArray)로 복제, 인스턴스는 SubObject 시스템에 등록되어 복제. 변경 통지는 서버 변경 경로와 클라 복제 콜백이 `Notify*FromList/Source` 진입점으로 수렴

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 모듈의 중심. Add/Consume/Use/Equip 의미와 FastArray·통지 구조가 모두 여기 정리됨
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템 행동이 어떻게 컴포지션되는지(6종 Fragment) 한눈에 파악
3. `Source/WxInventory/Public/Items/WxItemDefinition.h` / `WxItemInstance.h` — 정의(공유) vs 인스턴스(가변·복제) 분리 모델

## 관련
- 상위: [[WxUI]](인벤토리 표시), [[WxWorld]](보물상자 등 `IWxInteractionSource` 오너), 게임 모듈(장착 외형 반영 바인딩)
- 함께 보기: [[WxCore]](`IWxInteractionSource`)

---
*문서 기준 커밋 `6e6d0ae` · 생성일 2026-06-18 · 소스 17파일 — `/readme-writer`로 갱신*
