# WxInventory — 아이템·인벤토리 시스템

> 아이템의 정적 정의(Fragment 컴포지션)와 런타임 인스턴스, 서버 권위 인벤토리·장비 관리, 보상 지급까지 아이템 수명 전체를 담당하는 플러그인. 인벤토리는 FastArray 로 클라이언트에 복제된다.

## 책임
**담당**
- 아이템 정적 정의(`UWxItemDefinition`)와 Fragment 컴포지션(Stackable/Usable/Charges/Equippable/Pickup/Grade)
- 런타임 인스턴스(`UWxItemInstance`)의 생성·소멸·복제, 인스턴스별 충전량(에스트병 방식)
- 서버 권위 인벤토리 관리: 추가/차감/스택 머지·분할·사용, FastArray 기반 복제(`UWxInventoryManagerComponent`)
- 장비 착용 상태 보관과 EquipEffect GE 라이프사이클(`UWxEquipmentComponent`)
- 보상 데이터테이블(`FWxRewardTableRow`) 지급 진입점 — 픽업 월드 드랍 또는 인벤토리 직접 지급(`UWxRewardLibrary`, StateTree Task)

**경계 (비담당)**
- 무기 외형 반영(메시 스왑/소켓 재부착): `UWxEquipmentComponent` 는 `OnEquipVisualChanged` 로 방송만, 실제 반영은 [[WxGame]] 캐릭터가 수행
- 픽업 상호작용 컴포넌트 배선: 컴포넌트는 상속 BP 에서 추가([[WxWorld]] 규약), 응답·프롬프트는 `AWxItemPickup` 이 [[WxCore]] `IWxInteractable` 로 직접 구현
- GAS 어빌리티/스탯 정의 자체 — GE 적용만 트리거([[WxCombat]] 도메인)
- 인벤토리 UI 표시 — [[WxUI]]

## 의존성
- **주요 의존**: `WxCore`(IWxInteractable), GameplayAbilities(GE 적용/ASC), StateTreeModule(보상 Task), NetCore(FastArray/서브오브젝트 복제), Niagara(픽업 이펙트, private)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅ — 참조 Wx 플러그인은 `WxCore` 뿐. 픽업/장비의 상호작용·외형 결합은 델리게이트/BP 상속으로 우회해 도메인 경계를 지킴(`WxWorld`·`WxGame` 언급은 참조가 아니라 회피 근거)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInventoryManagerComponent` | 인벤토리 진입점. 추가/차감/사용/장착요청·통지 허브. PlayerController 에 부착, `FindInventory` 로 임의 액터에서 조회 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` |
| `FWxInventoryList` / `FWxInventoryEntry` | 매니저 내부 FastArray 슬롯 컬렉션·엔트리. 실제 머지/분할/차감 로직 | 같은 파일 |
| `UWxItemDefinition` | 아이템 정적 정의(PrimaryDataAsset). Category + Fragment 컨테이너 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` |
| `UWxItemFragment` | Fragment 베이스 + 6종 파생(Stackable/Usable/Charges/Equippable/Pickup/Grade) | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` |
| `UWxItemInstance` | 런타임 아이템 단위. 충전량 보유, 슬롯 델리게이트 안정 식별자, GA SourceObject | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h` |
| `UWxEquipmentComponent` | 장착 ItemDef 보관·복제 + EquipEffect GE 관리. 폰에 부착 | `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h` |
| `UWxRewardLibrary` | 보상 지급 서버 권위 진입점(픽업 드랍/직접 지급 분기). 상태 없는 1회성 | `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h` |
| `AWxItemPickup` | 월드 드랍 픽업 액터(Abstract). 상호작용 시 인벤토리 지급 후 파괴 | `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h` |

## 확장 포인트 / 규약
- **새 아이템**: `UWxItemDefinition` 데이터 자산 생성 → `Category`(EWxItemCategory) 지정 → `Fragments` 배열에 필요한 Fragment 를 EditInline 조합. 정적 정의는 공유, 가변 상태는 `UWxItemInstance` 보유
- **새 행동 축**: `UWxItemFragment` 상속(EditInlineNew). 인스턴스 초기 상태 주입이 필요하면 `OnInstanceCreated` 오버라이드(예: Charges 가 MaxCharges 로 시드). 소비처는 `FindFragmentByClass<T>()` 로 조회
- **스택 규약**: `Stackable` Fragment 있으면 한 슬롯 MaxStack 까지 머지 후 초과분 분할, 없으면 1슬롯=1개 강제
- **충전형(에스트병)**: `Charges` + `Usable` 조합. 사용 시 인벤토리 스택 유지, 인스턴스 충전량만 감소. `RefillItemCharges` 로 회복
- **보상 데이터 주도**: `FWxRewardTableRow`(최대 5 항목) DataTable Row. `Pickup` Fragment 유무로 월드 드랍/직접 지급이 갈림
- **권한 모델**: Add/Consume/Use/Equip 은 모두 서버 권위. FastArray + 복제 서브오브젝트로 클라 동기화. 서버 변경 경로와 클라 OnRep 콜백이 매니저의 `Notify*FromList/FromSource` 진입점으로 수렴해 델리게이트 발행

## 여기서부터 읽어라
1. `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h` — 시스템 전체 API·통지 흐름·스택 규약이 한곳에 정리된 허브
2. `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h` — Fragment 6종으로 아이템 기능 축 파악, 정의/인스턴스 관계 이해
3. `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp` — 머지/분할/차감/사용의 실제 구현과 복제 콜백 배선

## 관련
- 상위: [[WxGame]](장비 외형 반영·픽업 상호작용 BP), [[WxWorld]](상호작용 컴포넌트), [[WxUI]](인벤토리 표시), [[WxQuest]]·[[WxAI]](StateTree 보상 지급)

---
*문서 기준 커밋 `10f1722` · 생성일 2026-07-23 · 소스 19파일 — `/readme-writer`로 갱신*
