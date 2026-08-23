# WxInventory — 코드 리뷰

> 여전히 건강하다 — 권한 `check`, FastArray 더티 마킹, 서브오브젝트 등록/해제, 통지 진입점 수렴이 일관되고 규칙 위반(Copyright·람다·BlueprintCallable·플러그인 의존)은 실질적으로 없다. 남은 결함은 클라이언트 복제 콜백의 **통지 시점**과 추가/차감 경로의 **통지 입도 비대칭** 둘이다. 이번 리뷰는 매니저·인스턴스·Fragment·장비·보상·픽업·ST 태스크의 cpp 까지 통독했고, 복제 순서는 UE 5.8 엔진 소스(`FastArraySerializer.h`·`DataChannel.cpp`)로, 소비 영향은 WxGame 뷰모델·어빌리티로 대조 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 클라이언트 슬롯 제거 통지가 실제 제거 "전"에 발행되어, 풀(pull) 구독자가 제거된 슬롯을 살아 있는 것으로 본다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:44-70` (`FWxInventoryList::PreReplicatedRemove`), 대조: `:393-406` (`ConsumeItemsByDefinition`), `:371-376` (`RemoveItemInstance`)
- **범주**: 버그/정확성
- **문제**: 서버 경로는 엔트리를 **제거한 뒤** 통지한다. 클라이언트 경로는 반대다 — 엔진이 삭제 콜백을 먼저 부르고 실제 제거는 마지막에 한다(UE 5.8 `FastArraySerializer.h:1134` "Call the delete callbacks now, actually remove them at the end" → `:1148` PreReplicatedRemove → `:1165`/`:1176` PostReplicatedAdd/Change → `:1193` `RemoveAtSwap`). 즉 델리게이트가 나가는 순간 그 엔트리와 Instance 가 `Entries` 에 그대로 남아 있다. `:64-65`에서 `StackCount` 를 0 으로 내려 총합(`GetTotalItemCountByDefinition`)은 맞춰 두었지만, `GetAllItems()`(`:463`)·`FindFirstItemStackByDefinition()`(`:409`)·`FindUsableInstance()`(`:643`) 같은 풀 API 는 여전히 그 Instance 를 돌려준다. 실제 소비자가 이 경로를 탄다: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:97` `HandleStackChanged` → `:114` `RefreshAllItems()` → `:125` `GetAllItems()` 로 목록을 재구성하므로, 원격 클라이언트에서는 제거된 슬롯의 Item VM 이 수량 0 으로 잔존하다가 다음 인벤토리 변경 때까지 사라지지 않는다. 리슨 서버 호스트는 서버 경로라 정상이라 원격 클라에서만 재현되는 불일치다.
  현재 콘텐츠로는 트리거되지 않는다 — 슬롯이 완전히 비는 경로는 `ConsumeItemsByDefinition` 과 호출부 0건인 `RemoveItemInstance` 뿐인데, 유일한 소비 어빌리티가 충전형 전용(`Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.h:41-43`)이라 스택이 줄지 않는다. 비충전 소비 아이템 자산을 하나 만드는 순간 켜지는 잠복 결함이다.
- **제안**: `FWxInventoryList` 에 `PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters&)` 를 구현한다(같은 헤더 `:705` 정의, `:1619`에서 배열 정리 뒤 호출된다). `PreReplicatedRemove` 는 (Instance, ItemDef, Delta) 를 `NotReplicated` 멤버 큐에 쌓기만 하고, `PostReplicatedReceive` 에서 순서대로 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 를 발행하면 서버와 동일하게 "제거 완료 후 통지" 가 된다. 대안(풀 API 가 `StackCount<=0` 엔트리를 거르기)은 계약을 암묵적으로 만들어 권하지 않는다.
- **확신도**: 높음

### 2. 🟡 추가 경로만 정의 단위 스택 통지를 슬롯 수만큼 쪼개 발행한다 — 차감 경로와 비대칭
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:296`, `:312` (루프 안), 대조: `:405` (`ConsumeItemsByDefinition`, 전체 1회)
- **범주**: 설계/구조
- **문제**: `ConsumeItemsByDefinition` 은 슬롯 변경은 슬롯별로, 정의 단위 합계 변경(`NotifyStackChangedFromList`)은 `-NumToConsume` 한 번으로 발행한다. `AddItemDefinition` 은 머지 루프(`:296`)와 신규 청크 루프(`:312`) 양쪽에서 매 슬롯마다 정의 단위 통지를 낸다. 같은 델리게이트가 한쪽은 연산 1회당 1번, 한쪽은 슬롯 수만큼 나가는 셈이다.
  결과는 관측 가능하다: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-112` 가 `Delta > 0` 마다 새 `UWxViewModel_Item`(`AcquiredCount = Delta`)을 만들어 `LastAcquiredItem` 에 꽂고, 그 프로퍼티가 획득 토스트를 띄운다(`Source/WxGame/MVVM/WxViewModel_Inventory.h:77-81`). 비스택 아이템 5개를 한 번에 지급하면 "x1" 토스트가 5번 뜨고, 스택 상한을 넘겨 지급하면 "x2"·"x3" 으로 쪼개져 뜬다. 덤으로 `:114` `RefreshAllItems()`(내부 VM 매칭이 O(N²))도 그만큼 반복된다.
- **제안**: `AddItemDefinition` 도 슬롯 단위 통지(`NotifySlotChangedFromList`)만 루프 안에 두고, 정의 단위 `NotifyStackChangedFromList(ItemDef, StackCount)` 는 함수 끝에서 실제 추가된 총량으로 1회만 발행한다. 차감 경로와 입도가 맞춰지고 토스트·리프레시도 1회로 수렴한다.
- **확신도**: 중간(토스트 다중 발생은 확실, 슬롯별 발행이 의도였을 가능성은 낮게 봄)

### 3. 🟡 StateTree 태스크가 보상·리필 대상을 0번 PlayerController 로 고정한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 권위 측에서만 도는데(`:31`/`:31`) 대상은 `UGameplayStatics::GetPlayerController(Owner, 0)` 이다. 데디케이티드 서버·다중 클라이언트에서는 "서버 월드의 첫 컨트롤러" 가 상호작용한 플레이어와 무관하므로 비-픽업 보상(재화 등)과 에스트병 리필이 엉뚱한 플레이어에게 간다. 모듈 전체가 서버 권한 + FastArray 복제로 멀티를 전제하고 있는데 이 두 진입점만 싱글 전제다.
  이전 리뷰가 대비 사례로 들었던 `Source/WxGame/Character/WxEnemyCharacter.cpp` 도 현재는 `:109` 에서 같은 0번 컨트롤러를 쓴다 — 즉 프로젝트 전역이 같은 전제이며, 헤더 주석(`Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h:21` "로컬 플레이어(0번 컨트롤러)")도 이를 명시한다. 지금 범위에서는 의도로 보인다.
- **제안**: 장치 ST 컨텍스트가 상호작용자를 알고 있으므로, 인스턴스 데이터에 대상 액터를 두고 바인딩으로 받아 `FindInventory(Target)` 에 넘기는 것이 정공법이다. 당장 안 고칠 거면 멀티 활성화 체크리스트에 두 위치를 올려 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 장비 경로(EquipItemByDef → UWxEquipmentComponent)와 RemoveItemInstance 가 트리거 없는 데드 코드로 남아 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:584-610` (`EquipItemByDef`), `:345-377` (`RemoveItemInstance`), `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:28-30`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체 grep 결과 두 API 모두 호출부가 0건이며(BlueprintCallable 도 아니라 BP 진입도 없다), 헤더 주석이 이를 "미구현" 으로 정직하게 적어 두었다. 다만 `WxEquipmentComponent.h:30` 이 스스로 지적하듯, 늦게 relevant 해진 클라이언트는 `OnRep_EquippedItemDef`(`WxEquipmentComponent.cpp:60`)가 캐릭터 구독(`Source/WxGame/Character/WxCharacterBase.cpp:83`)보다 먼저 오면 외형 방송을 유실하는데 현재 상태를 되물을 pull API 가 없다. 경로를 살리는 순간 이 결함이 그대로 켜진다.
- **제안**: 완성할지 제거할지 결정한다. 완성 시 `UWxEquipmentComponent` 에 현재 장착 상태를 되묻는 getter 를 추가해 구독 직후 1회 당겨 오게 하고, 당분간 안 쓸 거면 `RemoveItemInstance` 와 함께 지워 API 표면을 줄인다.
- **확신도**: 높음(데드 코드 사실) / 낮음(제거 여부는 로드맵 판단)

### 5. 🟢 클라이언트 엔트리 제거는 RemoveAtSwap 이라 서버와 슬롯 순서가 달라진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:145-156` (`RemoveEntry`), `:166-198` (`ConsumeByDefinition` — 둘 다 `RemoveCurrent` 로 순서 보존), 소비처 `:409-425` (`FindFirstItemStackByDefinition`), `:643-669` (`FindUsableInstance`)
- **범주**: 버그/정확성
- **문제**: 서버는 순서를 보존하지만 클라이언트 FastArray 수신은 `Items.RemoveAtSwap`(`FastArraySerializer.h:1193`) 으로 지운다. 제거가 한 번이라도 일어난 뒤에는 같은 ItemDef 의 다중 슬롯 순서가 서버/클라에서 달라져, 클라 뷰모델이 `FindFirstItemStackByDefinition` 으로 고른 인스턴스의 충전량·아이콘이 서버가 `FindUsableInstance` 로 실제 사용할 인스턴스와 어긋날 수 있다. 충전형이 인벤토리에서 제거되지 않는 현 설계에서는 사실상 발생하지 않는다.
- **제안**: 지금 고칠 필요는 없다. 슬롯 순서에 의미를 부여하는 기능(정렬 고정·퀵슬롯 인덱스 등)을 붙일 때 서버가 순서 키를 엔트리에 복제하거나 클라가 ItemDef 단위로 안정 정렬하도록 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 StateTree 태스크 헤더의 `GetInstanceDataType()` 인라인 정의는 코딩 규칙 6 의 명시 위반이다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h:49`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h:33`
- **범주**: 규칙 위반
- **문제**: 두 헤더가 `virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }` 를 본문째 정의한다. 각 파일 상단 주석(`:13`/`:12`)이 "옮길 본문이 없다" 며 스스로 예외를 선언했지만, `Public/Items/WxItemDefinition.h:60-66` 의 템플릿과 달리 이 한 줄은 cpp 로 내리는 데 기술적 제약이 없다. 엔진 StateTree 관례를 따른 것이라 의도일 수 있다.
- **제안**: 규칙을 지킬 거면 cpp 로 옮기고, 예외로 둘 거면 `CLAUDE.md` 에 "ST 태스크 `GetInstanceDataType` 은 예외" 를 한 줄 명시해 파일마다 주석으로 변명하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 7. 🟢 픽업 프롬프트가 상호작용 키 "[F]" 를 문자열에 박아 넣는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:106-108`
- **범주**: 버그/정확성
- **문제**: 프롬프트 문자열이 `"[F] {0} x{1}"` 로 키 표기를 직접 포함한다. 키를 리매핑하거나 게임패드로 플레이하면 화면이 거짓 정보를 띄우고, 로컬라이즈 시에도 키가 번역 문자열에 묶인다. 같은 계약을 구현하는 다른 대상들은 키를 문자열에 넣지 않는다 — `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:82-86` 은 ST 가 세팅한 프롬프트 데이터를 그대로 답하고, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:27-30` 은 컴포넌트에 위임한다. 픽업만 키를 하드코딩한다.
- **제안**: 프롬프트는 아이템 이름·수량만 답하고, 키 표기는 표시하는 UI 가 입력 매핑에서 뽑아 붙이도록 한다(다른 대상들과 같은 계약이 된다).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`
- **훑은 파일**: `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`; 외부 대조: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, UE 5.8 `FastArraySerializer.h`·`DataChannel.cpp`·`NiagaraComponent.cpp`
- **미검토 / 한계**: 정적 분석만 수행했고 PIE 원격 클라이언트로 1번 항목을 실제 재현하지는 않았다. 검토 중 의심했다가 엔진 소스로 무혐의 확인한 두 건은 기록만 남긴다 — (a) 픽업이 `SpawnActorDeferred` 중 미등록 Niagara 컴포넌트를 `Activate` 하는 건 `bAutoActivate=true`(`NiagaraComponent.cpp:685`) 덕에 등록 시 재활성화된다, (b) FastArray 안의 ItemInstance 참조가 미해결(unmapped)로 도착해 통지를 잃을 위험은 엔진이 컴포넌트 서브오브젝트를 컴포넌트 본체보다 먼저 쓰기 때문에(`DataChannel.cpp:4368-4381`) 발생하지 않는다. BP/DataTable 에셋(ItemDef·보상 로우·픽업 BP)은 범위 밖이다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 22파일 — `/module-review`로 갱신*
