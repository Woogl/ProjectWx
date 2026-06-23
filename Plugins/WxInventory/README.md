# WxInventory — 아이템/인벤토리 시스템

> 아이템의 정적 정의(Definition + Fragment 컴포지션)와 런타임 인스턴스를 관리하고, 인벤토리 보관·사용·장착, 픽업/보상 드랍까지 아이템 수명 전반을 책임지는 Runtime 플러그인.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 데이터·행동을 선언하고, 런타임 인스턴스(`UWxItemInstance`)의 생성·소멸·복제를 관장한다.
- 인벤토리 보관: FastArray 기반 슬롯 스택 머지/분할(`UWxInventoryManagerComponent`), 정의 합계 조회·차감, 충전형(에스트병) 사용/리필.
- 장비: 장착 ItemDef 보관·복제와 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`).
- 보상 드랍: 픽업 액터 스폰·발사(`UWxRewardComponent`, `AWxItemPickup`), DataTable 기반 보상 정의(`FWxRewardTableRow`), StateTree 트리거(`FWxStateTreeTask_GrantReward`).

**경계 (비담당)**
- 무기 메시 스왑/소켓 재부착 등 실제 외형 반영 — `UWxEquipmentComponent`가 `OnEquipVisualChanged`로 방송만 하고, 게임 모듈(캐릭터)이 반영한다.
- 상호작용 트리거 — 픽업/보상은 WxCore의 `IWxInteractionSource`에 바인딩만 하며, 상호작용 컴포넌트 자체는 [[WxWorld]] 도메인 소속(픽업 BP에서 상속 추가).
- GameplayEffect 정의/적용 규칙은 GameplayAbilities 엔진 서브시스템에 위임(여기선 Spec 생성·ASC 적용만).

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractionSource`, `WxCollisionChannels`), GameplayAbilities/GameplayTags(GE 적용·ASC), StateTree(보상 Task), NetCore(FastArray 복제), Niagara·DeveloperSettings(private).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxWorld` 언급은 주석상 설명일 뿐, 코드는 WxCore의 인터페이스로만 결합)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Fragment 컬렉션 보유 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | EditInline 컴포지션 단위(Equippable/Usable/Charges/Stackable/Pickup/Grade) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 아이템 한 자루의 런타임 인스턴스. 충전량 보유, GA SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxInventoryManagerComponent` | 인벤토리 본체. Add/Consume/Use/Equip + FastArray 복제 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 복제 + EquipEffect GE 라이프사이클, 외형 방송 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardComponent` | 보상 드랍 스포너(픽업 스폰/발사, 비-픽업 직접 지급) | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardComponent.h` |
| `AWxItemPickup` | 월드 픽업 액터(상호작용 시 인벤토리 지급 후 파괴) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(아이템×수량 최대 5쌍) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **새 아이템 행동 추가**: `UWxItemFragment`를 상속한 새 Fragment 클래스를 만들고, 초기 인스턴스 상태가 필요하면 `OnInstanceCreated`를 오버라이드한다. 소비처는 `FindFragmentByClass<T>()`로 조회한다. 카테고리(`EWxItemCategory`)는 Fragment가 아니라 `UWxItemDefinition::Category`가 직접 표현한다(기능 축과 직교).
- **스택/충전 규약**: `Stackable` Fragment가 있어야 슬롯 머지(MaxStack), 없으면 1슬롯=1개. `Charges` Fragment 아이템은 사용 시 스택 대신 인스턴스 충전량을 차감하고 `RefillItemCharges`로 회복.
- **데이터 주도 설정**: 아이템은 `UWxItemDefinition` 데이터 자산, 보상은 `FWxRewardTableRow` DataTable(`RowType=/Script/WxInventory.WxRewardTableRow`). 기본 지급 아이템은 매니저의 `DefaultItems`.
- **권한 규약**: Add/Consume/Use/Equip 변경 경로는 서버 권한 전용. 클라이언트는 FastArray + SubObject 복제로 추종하며, 변경 통지는 `Notify*FromList`/`Notify*FromSource`로 서버·OnRep 경로가 수렴해 델리게이트(`OnInventoryStackChanged`/`SlotChanged`/`ChargeChanged`)를 발행한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Definition+Fragment 컴포지션 모델 전체. 모듈의 데이터 설계 축이 여기 다 있다.
2. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 슬롯/리스트/매니저의 제어 흐름과 복제·통지 경로. 인벤토리 동작의 단일 진입점.
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — Add 머지/분할, Use·Charges·Equip의 실제 분기 구현.

## 관련
- 상위: 인벤토리 매니저는 PlayerController에 부착되어 게임 모듈([[WxGame]])·UI([[WxUI]])가 소비한다. 장착 외형은 [[WxGame]] 캐릭터가, 픽업/보상 상호작용은 [[WxWorld]]가 WxCore 인터페이스를 통해 연결한다.

---
*문서 기준 커밋 `f89158d` · 생성일 2026-06-22 · 소스 18파일 — `/readme-writer`로 갱신*
