# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(DataAsset)와 런타임 인스턴스를 분리해, PlayerController 부착 컴포넌트로 소유·소비·장착·복제를 서버 권위로 관리하고, 보상 지급(픽업 스폰/직접 지급)까지 담당한다.

## 책임
**담당**
- 아이템 정의(`UWxItemDefinition`)와 기능 축 Fragment 컴포지션(Equippable/Usable/Charges/Stackable/Pickup/Grade)
- 런타임 인벤토리 소유·스택·소비·충전(에스트병식 Charges) — FastArray 기반 서버→클라 복제
- 보상 테이블(`FWxRewardTableRow`) 지급: 픽업 액터 스폰·발사 또는 인벤토리 직접 지급
- 아이템 사용(Usable GE 적용 + 스택/충전 차감), 장착 GE 라이프사이클(`UWxEquipmentComponent`)
- StateTree 태스크로 보상 지급·충전 리필을 콘텐츠(퀘스트/체크포인트)에 노출

**경계 (비담당)**
- 어빌리티/GameplayEffect 정의 자체 — 아이템은 GE 클래스를 참조만 하고 적용은 소유 폰 ASC에 위임 ([[WxCombat]])
- 상호작용 스캔·프롬프트 트리거 — 픽업은 `IWxInteractable`(WxCore 계약)만 구현, 스캐너는 [[WxWorld]]
- 인벤토리 UI/뷰모델 — 델리게이트만 발행 ([[WxUI]])
- 컴포넌트 부착 — 코드가 아니라 Experience 에셋 주입 설정으로 처리

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryComponent` | PlayerController 부착, Add/Consume/Use/Equip의 서버 권위 진입점 | `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `FWxInventoryList` | FastArraySerializer 복제 컨테이너, 실제 엔트리 변경 로직 | `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` |
| `UWxItemDefinition` | PrimaryDataAsset 아이템 정의 + Fragment 컬렉션 | `Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemInstance` | 개별 아이템 수명/식별 단위, 충전량 복제, GE SourceObject | `Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxItemFragment` | 기능 축 컴포지션 베이스(6종 파생) | `Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxRewardLibrary` | 보상 지급 서버 권위 진입점(픽업 스폰/직접 지급) | `Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 지급용 픽업 액터, `IWxInteractable` 구현 | `Source/WxInventory/Public/Items/WxItemPickup.h` |
| `UWxEquipmentComponent` | 폰 부착, 장착 ItemDef 보관 + EquipEffect GE 라이프사이클 | `Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxGameFeatureAction_AddInventoryItems` | Experience 액션셋에 실려 시작 아이템을 지급하는 GameFeatureAction | `Source/WxInventory/Public/Inventory/WxGameFeatureAction_AddInventoryItems.h` |

## 확장 포인트 / 규약
- 새 아이템 기능: `UWxItemFragment` 파생 후 정의 자산의 `Fragments`에 EditInline 부착. `OnInstanceCreated`로 인스턴스 초기 상태 주입. `FindFragmentByClass<T>()`로 조회.
- 데이터 주도: 아이템은 `UWxItemDefinition`(NotBlueprintable PrimaryDataAsset), 보상은 `FWxRewardTableRow` DataTable, `Item`은 `TSoftObjectPtr`라 지급 시점 지연 로드.
- 시작 아이템: 액션셋 `Actions`의 `UWxGameFeatureAction_AddInventoryItems`(Items)가 지급. 활성 시 이미 있는 인벤토리는 즉시, 이후 생기는 인벤토리는 `OnAnyInventoryReady`로, 서버 권한만. GameMode 등 외부 호출 없음.
- 리플리케이션/권한: Add/Consume/Use/Equip/Grant는 모두 서버 권위 전용. `FWxInventoryList`(FastArray)로 델타 복제되고, 각 `UWxItemInstance`는 `RegisterReplicatedInstance`로 서브오브젝트 복제. 변경은 정의 합계/슬롯/충전 3종 멀티캐스트 델리게이트로 브로드캐스트(관찰자가 먼저 존재할 수 있어 `OnAnyInventoryReady`는 클래스 static).
- StateTree: `FWxStateTreeTask_GiveRewards`·`FWxStateTreeTask_RefillItemCharges`는 라이브 전이 + 서버 권위에서만 실행(초기 진입/복원/레이트조인 시 중복 지급 방지).
- 미배선: `EquipItemByDef`/`UWxEquipmentComponent::EquipItem` 경로는 호출부가 없어(BlueprintCallable도 아님) 장비 경로 전체가 배선만 있고 트리거가 없다. `RemoveItemInstance`도 호출부 0건.

## 여기서부터 읽어라
1. `Source/WxInventory/Public/Inventory/WxInventoryComponent.h` — 소유·소비·사용·복제·델리게이트가 모두 모이는 허브. `FWxInventoryList`의 AddEntry/ConsumeByDefinition이 실제 변경 로직.
2. `Source/WxInventory/Public/Items/WxItemFragment.h` — 6종 Fragment로 아이템 기능 축이 어떻게 나뉘는지, 특히 Charges(에스트병식)와 Stackable/Usable의 직교 관계.
3. `Source/WxInventory/Public/WxRewardLibrary.h` — 픽업 스폰 vs 직접 지급으로 갈리는 보상 지급 흐름의 진입점.

## 관련
- 상위: 보상 지급을 트리거하는 [[WxQuest]]·체크포인트(StateTree 태스크), 아이템 사용 GE의 [[WxCombat]], 픽업 상호작용의 [[WxWorld]], 인벤토리 표시의 [[WxUI]]. 공용 계약(`IWxInteractable` 등)은 [[WxCore]].

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 22파일 — `/readme-writer`로 갱신*
