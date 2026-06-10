# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(데이터 자산)와 런타임 인스턴스를 관리하고, 플레이어 인벤토리의 추가·차감·사용·장착을 FastArray 레플리케이션으로 동기화하는 시스템.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`)와 Fragment 컴포지션으로 속성/행동 선언
- 아이템 런타임 인스턴스(`UWxItemInstance`) 생성·소멸·레플리케이션 및 인스턴스별 충전량 관리
- 인벤토리 슬롯 컬렉션(`FWxInventoryList`) 관리: 스택 머지/분할, 정의 합산 차감, FastArray 동기화
- 아이템 사용(`UseItemByDef`)·충전 리필(`RefillItemCharges`)·장착 요청(`EquipItemByDef`)의 권한 처리
- 정의 합계/슬롯/충전 단위 변경 델리게이트 브로드캐스트
- 보상 데이터테이블 Row 구조체(`FWxRewardTableRow`) 정의

**경계 (비담당)**
- 실제 장착 시각/부착 처리는 게임 측(`IWxEquipmentInterface` 구현체, `WxGame` Character)에 위임
- 인벤토리 UI 표현/바인딩은 [[WxUI]]에 위임 (본 모듈은 델리게이트만 제공)
- GameplayEffect 정의·적용 결과(스탯 변화)는 GAS([[WxCombat]])에 위임
- 픽업 액터 스폰·드롭 배치는 [[WxWorld]]에 위임 (본 모듈은 `Pickup` 데이터만 기술)

## 의존성
- **주요 의존**: `WxCore`, `GameplayAbilities`(`UseItemByDef`의 GE 적용), `NetCore`/FastArraySerializer(레플리케이션), `Niagara`(Pickup 데이터)
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 인벤토리 매니저 컴포넌트. 추가/차감/사용/장착의 권한 진입점, 변경 델리게이트 보유. `FindInventory`로 조회 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | 엔트리 컬렉션(FastArray)과 슬롯(인스턴스+StackCount). 머지/분할/차감 로직 | 동상 |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Fragment 컴포지션 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 아이템 런타임 인스턴스. 정의 바인딩 + 충전량 보유, GA SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | Fragment 베이스(EditInline). 하위: `_Equippable`/`_Usable`/`_Charges`(Refill)/`_Stackable`/`_Pickup` | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `IWxEquipmentInterface` | 장착 요청을 게임 측으로 플러그인 의존 없이 전달 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentInterface.h` |
| `FWxRewardTableRow` | 보상(아이템·수량 Pair 최대 5개) 데이터테이블 Row. 아이템은 지연 로드 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 폴더 구성
- `Public/Inventory` — 인벤토리 매니저 컴포넌트, 장착 인터페이스
- `Public/Items` — 아이템 정의/인스턴스/Fragment, 보상 테이블 Row

## 확장 포인트 / 규약
- **새 아이템 추가**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`(EWxItemCategory) 지정 후 필요한 Fragment를 `Fragments`(Instanced/EditInline)에 조합한다. 카테고리는 UI/기능 1차 분기 축이고, Fragment는 기능 축으로 독립적이다.
- **새 행동 추가 / Fragment 조합 규약**: `UWxItemFragment`를 **직접** 상속한다. Fragment 서브클래스끼리는 상속 금지 — 평면 직교 조합만 허용한다. `FindFragmentByClass`가 `IsA` 기반이라 Fragment 간 상속이 있으면 형제/배열 순서에 따라 오매칭된다. 그래서 사용 효과(`_Usable`)와 충전 횟수(`_Charges`)는 서로 직교한 별개 Fragment이며, 충전형 소비 아이템은 둘을 **함께** 부착한다. 필요 시 `OnInstanceCreated`를 override해 인스턴스 초기 상태를 주입한다(예: Charges가 MaxCharges로 채움).
- **스택 규칙**: `_Stackable` Fragment의 `MaxStack`까지 기존 엔트리에 머지하고 초과분은 새 엔트리로 분할. Fragment 부재 시 항상 1슬롯=1개. 같은 정의가 여러 슬롯에 분산돼도 `GetTotalItemCountByDefinition`/`ConsumeItemsByDefinition`로 합산 조회·차감한다.
- **충전형 vs 소모형**: `_Charges` Fragment가 있으면 사용 시 인벤토리 스택은 유지하고 인스턴스 충전량만 1 감소(소진돼도 인벤토리에 남음), 없으면 스택 1 차감. 회복은 `RefillItemCharges`(체크포인트 리필).
- **권한/리플리케이션(최대 4인 멀티)**: 인벤토리는 PlayerController에 부착되며 `FindInventory`가 폰→컨트롤러를 거쳐 조회한다. Add/Consume/Use/Equip/Refill은 모두 서버(권한)에서만 호출되어야 한다. `FWxInventoryList`(FastArray) + `UWxItemInstance`(등록 SubObject)로 클라이언트 동기화하고, 서버 변경 경로와 클라이언트 복제 콜백(`PostReplicatedAdd/Change`, `PreReplicatedRemove`, `OnRep_CurrentCharges`)이 동일한 `Notify*` 진입점으로 수렴해 델리게이트를 발행한다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 인벤토리 API 전체와 머지/차감/통지 모델의 주석이 가장 풍부하다
2. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — 머지/분할 추가, 합산 차감, 사용/충전/장착의 실제 권한·검증·레플리케이션 로직
3. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 컴포지션으로 아이템 행동을 어떻게 선언하는지

## 관련
- 상위: [[WxCore]]
- 경계: [[WxUI]](표현), [[WxWorld]](픽업 액터/드롭), [[WxCombat]](장착 구현·GE), [[WxQuest]](보상 지급), `WxGame` Character(`IWxEquipmentInterface` 구현)

---
*문서 기준 커밋 `2983a08e` · 생성일 2026-06-11 · 소스 11파일 — `/readme-writer`로 갱신*
