# WxInventory — 인벤토리 시스템

> 아이템의 정적 정의(DataAsset)와 런타임 인스턴스를 분리해 소지·스택·사용·장착·보상 지급을 관장하는 도메인 플러그인. Fragment 컴포지션으로 아이템 행동을 선언하고, FastArray로 인벤토리를 복제한다.

## 책임
**담당**
- 아이템 데이터 모델: 정적 정의(`UWxItemDefinition`) + Fragment 컴포지션(속성/행동) + 런타임 인스턴스(`UWxItemInstance`)
- 인벤토리 소지/스택 머지·분할/정의 단위 차감의 서버 권위 처리와 FastArray 복제(`UWxInventoryManagerComponent`)
- Usable/Charges 기반 사용, Equippable 장착 효과(GE) 라이프사이클(`UWxEquipmentComponent`)
- DataTable Row 기반 보상 지급(픽업 드랍 또는 인벤토리 직접 지급)과 진입점(`UWxRewardLibrary`, `FWxStateTreeTask_GrantReward`, `AWxItemPickup`)

**경계 (비담당)**
- 무기 메시 스왑·소켓 재부착 등 실제 외형 반영 — 무기 액터가 게임 모듈 소유라 도메인에서 접근 불가. `OnEquipVisualChanged`로 방송만 하고 [[WxGame]]이 반영
- 픽업 상호작용 컴포넌트 배선 — `IWxInteractionSource`(WxCore)를 BeginPlay에 자동 바인딩, 컴포넌트 자체는 상속 BP가 추가([[WxWorld]])
- 인벤토리·아이템 UI 표현 — [[WxUI]]

## 의존성
- **주요 의존**: `WxCore`(상호작용 인터페이스 등), `GameplayAbilities`/`GameplayTags`(사용/장착 GE·ASC), `StateTree`(보상 Task), `NetCore`(FastArray 복제), `Niagara`(픽업 이펙트), `DeveloperSettings`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxWorld` 언급은 참조 회피를 설명하는 주석뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 소지/스택/사용/장착 요청의 권위 허브, FastArray 복제 소유. PlayerController 부착, `FindInventory`로 조회 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | 매니저 내부 FastArray 슬롯 컬렉션·엔트리(권한 변경 메서드 AddEntry/Consume…) | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Category + Fragment 컬렉션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | Fragment 베이스(EditInline) + Equippable/Usable/Charges/Stackable/Pickup/Grade 파생 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 슬롯 단위 식별·충전량을 가진 런타임 인스턴스(복제, GA SourceObject) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관·EquipEffect GE 라이프사이클, 외형은 방송만 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 Row 서버 권위 지급(픽업 스폰 또는 직접 지급)의 BP 라이브러리 | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 월드 드랍 픽업 액터, 상호작용 시 인벤토리 지급 후 파괴(Abstract, BP 상속) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `FWxStateTreeTask_GrantReward` | 라이브 전이 시 1회성 보상 지급 StateTree Task | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 데이터 자산 생성 → `Category`(EWxItemCategory) 설정 → `Fragments`에 필요한 Fragment를 EditInline으로 조합. 카테고리(UI/기능 1차 분기)와 Fragment("무엇을 할 수 있는가")는 직교하도록 설계한다.
- **새 Fragment**: `UWxItemFragment`(EditInlineNew, Abstract) 상속. 인스턴스 초기 상태 주입이 필요하면 `OnInstanceCreated` 오버라이드(예: `_Charges`가 MaxCharges 시드).
- **사용/장착 조합**: Usable(GE 적용), Charges(에스트병식 인스턴스 충전), Stackable(MaxStack 머지), Equippable(EquipEffects + 메시/소켓)을 조합. Stackable 부재 시 1슬롯=1개, Charges 단독 부착 시 스택 유지·충전만 소모.
- **보상 데이터**: `FWxRewardTableRow` DataTable(최대 5항목)에 Row 작성 후 `GrantReward`/StateTree Task/픽업으로 지급. `Item`은 `TSoftObjectPtr` 지연 로드. 매니저 `DefaultItems`는 BeginPlay 1회 자동 지급.
- **리플리케이션**: 슬롯은 `FWxInventoryList`(FFastArraySerializer), 인스턴스는 SubObject 등록. Add/Consume/Equip은 서버 권한에서만 호출하고, 서버 변경·클라 OnRep 양 경로가 `NotifyXxxFromList/Source` 진입점으로 수렴해 3종 델리게이트(Stack/Slot/Charge)를 발행한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — Add/Consume/Use/Equip 권위 API와 FastArray(`FWxInventoryList`)·통지 델리게이트 구조가 모듈의 중심
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 컴포지션이 아이템 행동을 결정하는 핵심 데이터 모델(Stackable↔Charges↔Usable 직교)
3. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 픽업 드랍 vs 직접 지급 분기와 외부(퀘스트/상자) 진입점

## 관련
- 상위: [[WxGame]](인벤토리·장비 컴포넌트 소유·바인딩), [[WxCore]](공용 정의)
- 협력: [[WxWorld]](상호작용), [[WxUI]](표시)
---
*문서 기준 커밋 `d0c804a` · 생성일 2026-07-12 · 소스 19파일 — `/readme-writer`로 갱신*
