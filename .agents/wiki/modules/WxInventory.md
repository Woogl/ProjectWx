# WxInventory — 아이템 및 인벤토리 시스템

> 상태: needs-review · 2026-09-20 이관 · 원문 기준 커밋: 047197a
> 기존 README를 이관했습니다. 전체 코드 재검증은 하지 않았습니다. 아래 과거 설명은 탐색에 사용하고 변경 전 원자료를 확인하세요. 에셋 내부는 미검증입니다.
> [Wiki 목차](../index.md) · [운영 절차](../maintenance.md)


> 아이템의 정의·인스턴스·소유를 관리하고, 획득(보상/픽업)·사용(소비)·충전(에스트병) 흐름을 서버 권위로 처리한다. 인벤토리 컴포넌트는 PlayerController에 부착되어 FastArray로 클라이언트에 복제된다.

## 책임
**담당**
- 아이템 정의(데이터 자산)와 런타임 인스턴스의 생성·소멸·복제
- 스택 머지/분할, 정의 단위 합산 차감, 슬롯/합계/충전 변경 통지
- 보상 테이블 기반 지급과 픽업 액터 스폰·발사
- 소비 아이템 사용(GameplayEffect 적용 + 스택/충전 차감)과 충전형 리필

**경계 (비담당)**
- 상호작용 스캔·프롬프트 표시는 [WxWorld](WxWorld.md)의 `UWxInteractionScannerComponent`가 담당하며, 픽업은 [WxCore](WxCore.md)의 `IWxInteractable` 계약만 구현한다(그래서 WxWorld를 참조하지 않는다)
- 인벤토리·충전 표시 UI와 그 뷰모델은 이 모듈 밖(`Source/WxGame/MVVM/WxViewModel_Inventory*`)에 있고, 여기서는 변경 델리게이트만 발행한다
- 아이템 사용을 발동하는 UseItem 어빌리티(GA)와 ASC 소유는 `WxGame` 쪽이며, 이 모듈은 GE 적용과 차감만 수행한다
- 사용 타이밍 판정은 애니메이션이 쥐고 있다 — 이 모듈은 AnimNotify가 쏜 GameplayEvent를 받는 쪽이다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | 소유·복제·사용·통지가 모이는 허브. 서버 권위 Add/Consume/Use/Refill의 유일한 진입점 | [Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h) |
| `FWxInventoryList` | 같은 헤더 안의 FastArraySerializer. 엔트리 저장·복제와 복제 콜백→컴포넌트 통지 변환을 맡는다 | [Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h) |
| `UWxItemDefinition` | 정적 정의(PrimaryDataAsset). Fragment 컬렉션으로 아이템 능력을 구성한다 | [Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h) |
| `UWxItemInstance` | 런타임 수명·식별 단위. 슬롯 델리게이트의 안정 식별자이자 사용 GE의 SourceObject | [Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h) |
| `UWxItemFragment` | 기능 축 컴포지션 베이스. Usable/Charges/Stackable/Pickup/Grade 파생이 한 헤더에 모여 있다 | [Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h) |
| `UWxRewardLibrary` | 아이템이 월드로 나가는 획득 경로의 진입점 — 픽업 스폰 vs 직접 지급 분기 | [Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h](../../../Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h) |
| `AWxItemPickup` | 지급용 픽업 액터. `IWxInteractable` 구현으로 상호작용 대상이 되고, 지급 후 파괴된다 | [Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h) |
| `UWxItemUseComponent` | 폰 쪽 중계기. 어빌리티가 예약한 아이템을 AnimNotify 이벤트 시점에 인벤토리로 넘긴다 | [Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxItemUseComponent.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxItemUseComponent.h) |

## 확장 포인트 / 규약
- **아이템 추가**: `UWxItemDefinition` 데이터 자산을 만들고 `Category`를 지정한 뒤 `Fragments`에 EditInline으로 능력을 조합한다. 정의는 `NotBlueprintable`(AssetManager PrimaryAssetType 정합성).
- **Fragment 기능 축**(정의 자산 안의 단일 객체를 모든 인스턴스가 공유): `Usable`(사용 시 GE), `Charges`(에스트병식 인스턴스별 충전 — 스택 대신 충전량 소모, 리필로 회복), `Stackable`(슬롯당 MaxStack 머지, 부재 시 슬롯당 1개), `Pickup`(픽업 액터/메시/나이아가라 데이터), `Grade`(등급·색). 새 능력은 `UWxItemFragment` 파생 + 필요 시 `OnInstanceCreated` 오버라이드.
- **리플리케이션 모델**(최대 4인 멀티): `FWxInventoryList`(FastArraySerializer)가 엔트리를 복제하고, `UWxItemInstance`는 서브오브젝트로 등록되어 개별 복제된다. Add/Consume/Use/Refill은 모두 서버 권한에서만 호출한다. 픽업의 `ItemDef`/`Quantity`는 `COND_InitialOnly`라 `SpawnActorDeferred`→`FinishSpawning` 사이에 주입해야 한다.
- **통지 단일화**: 서버 변경 경로와 클라이언트 복제 콜백(`PreReplicatedRemove`/`PostReplicatedAdd`/`PostReplicatedChange`, `OnRep_CurrentCharges`)이 모두 `Notify*FromList`/`FromSource`로 수렴해 같은 델리게이트(합계·슬롯·충전·전체 재읽기)를 발행한다. 뷰가 인벤토리보다 먼저 존재할 수 있어 수명은 클래스 정적 `OnAnyInventoryReady/Ended`로 관찰한다.
- **사용 흐름**: UI/입력 → (`RequestUseConsumable`이 `Ability.UseItem` 태그로 GA 발동) → 몽타주의 `UWxAnimNotify_UseItem`이 `Event.UseItem` 발행 → `UWxItemUseComponent`가 서버에서 수신 → `UWxInventoryComponent::UseItemByDef`. 태그는 이 모듈이 선언하지 않고 [WxCore](WxCore.md)의 `WxGameplayTags.h`에서 가져온다.
- **데이터 주도 설정**: 보상은 `FWxRewardTableRow` DataTable Row(항목 최대 5, 아이템은 소프트 참조 → 지급 시점 로드), 시작 지급은 컴포넌트의 `StartingItems`, 자동화는 StateTree 태스크 `FWxStateTreeTask_GiveRewards`·`FWxStateTreeTask_RefillItemCharges`. 두 태스크 모두 초기 진입(시작/복원/레이트조인)에서는 실행하지 않아 중복 지급·리필을 막는다.

## 여기서부터 읽어라
1. [Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h) — 엔트리 구조·FastArray·델리게이트·공개 API가 한 헤더에 모여 있어 모듈의 계약 전체가 여기서 잡힌다.
2. [Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h](../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h) — 아이템 능력이 어떻게 조합되는지 보면 정의/인스턴스의 역할 분담이 명확해진다.
3. [Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp](../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp) — 서버 변경과 복제 콜백이 어떻게 같은 통지로 합류하는지(델타 계산 포함) 확인한다.
4. [Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h](../../../Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h) — 아이템이 월드로 나가는 획득 경로의 분기 규칙.

## 관련
- 상위: `WxGame`이 PlayerController에 인벤토리를 붙이고, UseItem 어빌리티와 MVVM 뷰모델([Source/WxGame/MVVM/](../../../Source/WxGame/MVVM/))이 이 모듈의 API·델리게이트를 소비한다. 획득은 [WxWorld](WxWorld.md)(상호작용 스캔)와 StateTree 보상/리필 태스크에서 들어온다. 공용 계약(`IWxInteractable`, `WxGameplayTags`)은 [WxCore](WxCore.md).

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 24파일 — `/readme-writer`로 갱신*
