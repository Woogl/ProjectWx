# WxInventory — 아이템·인벤토리 시스템

> 아이템 정의(데이터 자산)와 런타임 인스턴스를 분리해, PlayerController 부착 컴포넌트가 획득·소비·충전·복제를 관장하고 보상 지급 경로를 제공한다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`)와 Fragment 컴포지션(Usable·Charges·Stackable·Pickup·Grade)
- 인벤토리 컴포넌트: FastArray 복제, 정의 단위 추가/소비, 슬롯·합계·충전 변경 통지
- 아이템 사용(GameplayEffect 적용 + 차감)과 충전형(에스트병) 충전/리필
- 보상 지급(`UWxRewardLibrary::GrantReward`): 픽업 액터 스폰·발사 또는 인벤토리 직접 지급
- 픽업 액터(`AWxItemPickup`)의 상호작용 지급 및 StateTree 지급/리필 태스크

**경계 (비담당)**
- 사용 어빌리티의 활성/판정·입력 라우팅 → [[WxCombat]] (GAS 어빌리티). 컴포넌트는 AssetTag로 발동만 위임
- 상호작용 스캔·프롬프트 표시 → [[WxWorld]]. `IWxInteractable` 계약은 [[WxCore]]에 있어 직접 참조하지 않음
- 인벤토리 UI 렌더/뷰모델 → [[WxUI]]. 컴포넌트는 델리게이트만 발행

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | PlayerController 부착. 추가/소비/사용/리필의 서버 권위 진입점, 복제·통지 허브 | `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Fragment 컬렉션 보유 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 개별 아이템의 런타임 수명·식별 단위(충전량 등 가변 상태), GE SourceObject | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | 정의에 EditInline 부착되는 기능 축 베이스(Usable/Charges/Stackable/Pickup/Grade) | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `FWxInventoryList` | `FFastArraySerializer` 기반 엔트리 목록. Add/Consume/Stack 실제 조작 담당 | `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxRewardLibrary` | 보상 로우 지급의 무상태 서버 진입점(픽업 스폰 vs 직접 지급 분기) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `FWxRewardTableRow` | DataTable 보상 로우(최대 5개 항목). Item은 지급 시점 지연 로드 | `Source/WxInventory/Public/Items/WxRewardTableRow.h` |
| `AWxItemPickup` | 월드 픽업 액터. 상호작용 시 인벤토리 지급 후 파괴 | `Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- 새 아이템: `UWxItemDefinition` 데이터 자산을 만들고 `Fragments`에 필요한 기능 축을 EditInline으로 조합한다. Category는 정의가 직접 표현하고 Fragment는 "무엇을 할 수 있는가"만 담당.
- 새 기능 축: `UWxItemFragment`를 상속해 `OnInstanceCreated`로 인스턴스 초기 상태를 주입(예: Charges가 충전량 시드). 정의당 단일 객체이므로 인스턴스별 가변 상태는 `UWxItemInstance`에 둔다.
- 데이터 주도: 보상은 `FWxRewardTableRow` DataTable 로우. `Item`은 `TSoftObjectPtr`라 테이블 로드 시 자산까지 끌어오지 않고 지급 시점에 동기 로드한다.
- 리플리케이션/권한: Add/Consume/Use/Refill은 서버 권한에서만. 목록은 `FWxInventoryList`(FastArray)로, 인스턴스는 개별 서브오브젝트 복제로 동기화. 클라 통지는 OnRep→`Notify*FromSource/List` 경로로 수렴한다.

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 시스템 전체의 진입점·델리게이트·권한 규약이 헤더 주석에 집약돼 있다
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템이 "무엇을 할 수 있는가"의 전 스펙트럼(5종 Fragment)이 한 파일에 있다
3. `Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp` — FastArray 조작·머지/분할·소비 원자성·통지 발행의 실제 구현

## 관련
- 상위: [[WxCore]] (공용 정의·`IWxInteractable` 계약)
- 소비처: [[WxCombat]] · [[WxWorld]] · [[WxUI]]

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 19파일 — `/readme-writer`로 갱신*
