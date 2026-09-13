# WxInventory — 아이템 및 인벤토리 시스템

> 아이템의 정의·인스턴스·소유를 관리하고, 획득(보상/픽업)·사용(소비)·충전(에스트병) 흐름을 서버 권위로 처리한다. 인벤토리 컴포넌트는 PlayerController에 부착되어 FastArray로 클라이언트에 복제된다.

## 책임
**담당**
- 아이템 정의(데이터 자산)와 런타임 인스턴스의 생성·소멸·복제
- 스택 머지/분할, 정의 단위 합산 차감, 슬롯/합계/충전 변경 통지
- 보상 테이블 기반 지급과 픽업 액터 스폰·발사
- 소비 아이템 사용(GameplayEffect 적용 + 스택/충전 차감)과 충전형 리필

**경계 (비담당)**
- 상호작용 스캔·프롬프트 표시는 [[WxWorld]]가 담당하며, 픽업은 [[WxCore]]의 `IWxInteractable` 계약만 구현한다
- 인벤토리/충전 표시 UI는 [[WxUI]] (여기서는 변경 델리게이트만 발행)
- 아이템 사용을 발동하는 UseItem 어빌리티(GA)와 ASC는 인벤토리 밖에 있고, 이 모듈은 GE 적용과 차감만 수행한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 소유·복제·사용의 중심 컴포넌트(서버 권위 Add/Consume/Use/Refill). 나머지 타입이 모이는 허브 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | 정적 정의(PrimaryDataAsset). Fragment 컬렉션으로 아이템 구성 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 런타임 수명·식별 단위. 슬롯 델리게이트의 안정 식별자이자 사용 GE의 SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스. Usable/Charges/Stackable/Pickup/Grade 파생 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxRewardLibrary` | 보상(RewardRow) 지급의 서버 진입점 — 픽업 스폰 또는 직접 지급 분기 | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 지급용 픽업 액터. `IWxInteractable` 구현, 상호작용 시 인벤토리에 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxItemUseComponent` | AnimNotify GameplayEvent를 받아 준비된 소비 아이템을 한 번 사용 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxItemUseComponent.h` |
| `FWxRewardTableRow` | 보상 DataTable Row(최대 5항목, 아이템은 소프트 참조) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h` |

## 확장 포인트 / 규약
- **아이템 추가**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`를 지정한 뒤 `Fragments`에 EditInline으로 능력을 조합한다. 정의는 `NotBlueprintable`(AssetManager PrimaryAssetType 정합성).
- **Fragment 기능 축**(각 아이템 인스턴스가 공유): `Usable`(사용 시 GE), `Charges`(에스트병식 인스턴스별 충전 — 스택 대신 충전량 소모, 리필로 회복), `Stackable`(슬롯당 MaxStack 머지), `Pickup`(픽업 액터/메시/나이아가라 데이터), `Grade`(등급·색; 기본 색 팔레트는 C++ `GetDefaultColorForGrade`에만 정의). 새 능력은 `UWxItemFragment` 파생 + 필요 시 `OnInstanceCreated` 오버라이드.
- **리플리케이션 모델**: `FWxInventoryList`(FastArraySerializer)가 엔트리를 복제하고, `UWxItemInstance`는 서브오브젝트로 등록되어 개별 복제된다. Add/Consume/Use/Refill은 서버 권한에서만 호출.
- **통지**: 정의 합계(`OnInventoryStackChanged`)·슬롯(`OnInventorySlotChanged`)·충전(`OnInventoryChargeChanged`)·전체 재읽기(`OnInventoryContentsChanged`) 델리게이트로 뷰가 갱신된다. 인벤토리 수명은 클래스 정적 `OnAnyInventoryReady/Ended`로 관찰(뷰모델이 인벤토리보다 먼저 존재 가능).
- **보상/충전 자동화**: StateTree 태스크 `FWxStateTreeTask_GiveRewards`(라이브 전이에서 RewardRow 지급), `FWxStateTreeTask_RefillItemCharges`(체크포인트 등에서 로컬 플레이어 충전 리필). 둘 다 초기 진입/복원/레이트조인에서는 중복 방지를 위해 실행하지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 소유·복제·사용·통지가 모두 여기서 정의되는 허브. 모듈 전체의 계약을 먼저 파악한다.
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — 아이템 능력이 어떻게 조합되는지(기능 축)를 보면 정의/인스턴스의 역할이 명확해진다.
3. `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` — 아이템이 월드로 나가는 획득 경로(픽업 스폰 vs 직접 지급)의 진입점.

## 관련
- 상위: 획득 흐름은 [[WxWorld]](상호작용)·StateTree(보상/리필 태스크)에서 이 모듈을 호출하고, 표시는 [[WxUI]]가 델리게이트를 구독한다. 공용 계약(`IWxInteractable` 등)은 [[WxCore]].

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 24파일 — `/readme-writer`로 갱신*
