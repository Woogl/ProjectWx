# WxInventory — 인벤토리 시스템

> 아이템의 정적 정의(데이터 자산)와 런타임 인스턴스를 분리해 소유·사용·장착·보상 지급을 관장하는 도메인 플러그인. 인벤토리는 FastArray 로 서버→클라 동기화된다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`) + Fragment 컴포지션으로 아이템의 속성/행동 선언
- 아이템 런타임 인스턴스(`UWxItemInstance`) 생성·소멸·레플리케이션, 인스턴스별 충전(Charges) 상태 관리
- 인벤토리 슬롯 컬렉션(`FWxInventoryList`): 스택 머지/분할, 정의 합산 차감, FastArray 동기화
- 아이템 사용(`UseItemByDef`)·충전 리필(`RefillItemCharges`)·장착 요청(`EquipItemByDef`) 권한 처리
- 정의 합계/슬롯/충전 단위 변경 델리게이트 발행
- 보상 지급(`UWxRewardComponent`)과 월드 픽업 액터(`AWxItemPickup`) 스폰/직접 지급

**경계 (비담당)**
- 장착의 실제 시각/부착 반영 → `UWxEquipmentComponent` 가 `OnEquipVisualChanged`(USkeletalMesh*, FName) 로 방송, 게임 측(`WxGame` Character)이 무기 메시 스왑/소켓 재부착 수행
- 상호작용 게이팅/프롬프트 → [[WxCore]] 의 IWxInteractionSource (BP 상속으로 픽업·보상 컴포넌트에 부착)
- GameplayEffect 정의·적용 결과(스탯 변화) → GAS / [[WxCombat]] 에 위임. 본 모듈은 Spec 적용만 트리거
- 인벤토리 UI 표현/바인딩 → [[WxUI]] (델리게이트만 제공)

## 의존성
- **주요 의존**: `WxCore`(공용 정의, IWxInteractionSource), `GameplayAbilities`(사용/장착 효과 GE), `GameplayTags`, `NetCore`/FastArraySerializer(레플리케이션), `DeveloperSettings`(등급 색상), `Niagara`(픽업 이펙트)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (장착은 엔진 타입 델리게이트 방송, 상호작용은 WxCore 인터페이스로 역참조 회피)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 추가/차감/사용/장착의 권한 진입점, 변경 델리게이트 보유. PlayerController 부착, `FindInventory` 로 조회 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | 엔트리 컬렉션(FastArray)과 슬롯(인스턴스+StackCount). 머지/분할/차감 로직 | 동상 |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Category + Fragment 컴포지션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | Fragment 베이스(EditInline). 하위: `_Equippable`/`_Usable`/`_Charges`(Refill)/`_Stackable`/`_Pickup` | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 아이템 한 자루의 런타임 인스턴스. 정의 바인딩 + 충전량 보유, GA SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 상태(`EquippedItemDef` 복제)·EquipEffect GE 수명 관리. 외형은 `OnEquipVisualChanged` 로 게임 측에 방송(역참조 회피) | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardComponent` | DataTable(`FWxRewardTableRow`) 기반 보상 드랍/직접 지급 스포너 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardComponent.h` |
| `AWxItemPickup` | 월드 픽업 액터. 상호작용 시 인벤토리에 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |

## 폴더 구성
- `Public/Inventory` — 인벤토리 매니저·보상 컴포넌트, 장착 컴포넌트
- `Public/Items` — 아이템 정의/인스턴스/Fragment(등급·색상은 `UWxItemFragment_Grade`), 픽업 액터, 보상 테이블 Row

## 확장 포인트 / 규약
- **새 아이템 추가**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`(EWxItemCategory) 지정 후 필요한 Fragment 를 `Fragments`(Instanced/EditInline)에 조합한다. 카테고리는 UI/기능 1차 분기 축, Fragment 는 기능 축으로 독립(직교)이다.
- **새 행동 추가 / Fragment 조합 규약**: `UWxItemFragment` 를 **직접** 상속한다. Fragment 서브클래스끼리는 상속 금지 — 평면 직교 조합만 허용한다(`FindFragmentByClass` 가 `IsA` 기반이라 Fragment 간 상속이 있으면 오매칭). 그래서 사용 효과(`_Usable`)와 충전 횟수(`_Charges`)는 별개 Fragment 이며, 충전형 소비 아이템은 둘을 **함께** 부착한다. 인스턴스 초기 상태는 `OnInstanceCreated` override 로 주입한다(예: Charges 가 MaxCharges 로 채움).
- **스택 규칙**: `_Stackable` 의 `MaxStack` 까지 기존 엔트리에 머지하고 초과분은 새 엔트리로 분할. Fragment 부재 시 항상 1슬롯=1개. 같은 정의가 여러 슬롯에 분산돼도 `GetTotalItemCountByDefinition`/`ConsumeItemsByDefinition` 로 합산 조회·차감(원자적).
- **충전형 vs 소모형**: `_Charges` 가 있으면 사용 시 인벤토리 스택은 유지하고 인스턴스 충전량만 1 감소(소진돼도 인벤토리에 남음), 없으면 스택 1 차감. 회복은 `RefillItemCharges`(체크포인트 리필).
- **데이터 주도**: 보상은 `FWxRewardTableRow`(DataTable, 아이템·수량 Pair 최대 5개, `TSoftObjectPtr` 지연 로드). 등급/색상은 `UWxItemFragment_Grade`(Grade + Color, 등급별 기본색은 C++ 생성자에서 시드, 기획자 오버라이드 가능).
- **권한/리플리케이션(최대 4인 멀티)**: 인벤토리는 PlayerController 부착, `FindInventory` 가 폰→컨트롤러를 거쳐 조회한다. Add/Consume/Use/Equip/Refill 은 모두 서버(권한) 전용. `FWxInventoryList`(FastArray) + `UWxItemInstance`(등록 SubObject)로 동기화하고, 서버 변경 경로와 클라 복제 콜백(`PostReplicatedAdd/Change`, `PreReplicatedRemove`, `OnRep_CurrentCharges`)이 동일한 `Notify*` 진입점으로 수렴해 델리게이트를 발행한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 API 전체와 머지/차감/통지 모델의 주석이 가장 풍부한 허브
2. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — 머지/분할 추가, 합산 차감, 사용/충전/장착의 실제 권한·검증·레플리케이션 로직
3. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 5종으로 아이템 행동을 어떻게 선언하는지

## 관련
- 상위: PlayerController/`WxGame` Character(`OnEquipVisualChanged` 바인딩해 외형 반영), [[WxUI]](표현), [[WxWorld]](상호작용 픽업/보물 상자), [[WxQuest]](보상 지급)

---
*문서 기준 커밋 `157ccd5` · 생성일 2026-06-13 · 소스 19파일 — `/readme-writer`로 갱신*
