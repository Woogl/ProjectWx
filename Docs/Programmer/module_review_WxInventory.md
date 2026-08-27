# WxInventory — 코드 리뷰

> 데이터 모델(Definition + Fragment 컴포지션)과 서버 권위 복제 경계가 Lyra 패턴에 충실하게 잡혀 있고, 프로젝트 코딩·모듈 규칙 위반은 기계적으로 확인한 범위에서 0건인 건강한 모듈이다. 이번 리뷰는 22개 소스를 전부 읽되 `FWxInventoryList` 복제 콜백·`UWxInventoryManagerComponent` 의 Add/Consume/Use 경로·`UWxEquipmentComponent` GE 수명·`UWxRewardLibrary` 드랍 경로를 라인 단위로 보고, 모듈 밖 호출부(WxGame 어빌리티·뷰모델, WxCore 계약)와 `Config/DefaultGame.ini`, UE 5.8 엔진 소스까지 대조해 검증했다. **직전 리뷰의 🔴(클라 아이템 추가 통지 유실)은 이번에 엔진 소스로 반증되어 제거했다** — 「미검토 / 한계」의 (a) 참조.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `GetPrimaryAssetId` 오버라이드가 `DefaultGame.ini` 의 아이템 스캔 설정을 통째로 무력화한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:14`, `Config/DefaultGame.ini:50`
- **범주**: 버그/정확성
- **문제**: 코드는 `FPrimaryAssetId(TEXT("WxItem"), GetFName())` 을 반환하는데, 설정은 `PrimaryAssetType="WxItemDefinition"`(AssetBaseClass `/Script/WxInventory.WxItemDefinition`, Directories `/Game/Item`)으로 등록돼 있다. 둘이 어긋나면 자산이 조용히 버려진다. 경로는 다음과 같이 확정된다.
  1. `UObject::GetAssetRegistryTags`(UE 5.8 `Obj.cpp:2516`)가 오버라이드된 ID 를 그대로 `PrimaryAssetType` 태그로 굽는다 → 태그 값이 `WxItem`.
  2. `UAssetManager::ScanPathsForPrimaryAssets`(`AssetManager.cpp:1415~1440`)는 태그 타입이 설정 타입과 다르면 `Ignoring PrimaryAssetType WxItemDefinition - Conflicts with WxItem` 로그를 남기고 해당 자산을 **스킵**한다.
  즉 `/Game/Item` 의 모든 `UWxItemDefinition`(DA_Potion·DA_Katana·DA_Gold)이 AssetManager 레지스트리에 하나도 등록되지 않으며, 설정 항목은 죽은 채로 남는다. `WxItem` 타입은 저장소 어디에서도 등록·소비되지 않는다(전역 grep 0건). 지금은 아이템을 AssetManager 로 조회하는 코드가 없어 런타임 파손은 없지만, 청크/번들/쿡 룰을 아이템에 걸려는 순간 조용히 실패한다.
- **제안**: 오버라이드(`WxItemDefinition.cpp:12~15`, `WxItemDefinition.h:54`)를 제거한다. `UPrimaryDataAsset::GetPrimaryAssetId()` 기본 구현(`DataAsset.cpp:121`)이 네이티브 클래스에 대해 `FPrimaryAssetId(GetClass()->GetFName(), GetFName())` = `WxItemDefinition:DA_Potion` 을 반환해 설정과 정확히 맞는다. `WxItem` 이라는 이름을 유지하고 싶다면 반대로 ini 의 `PrimaryAssetType` 을 `WxItem` 으로 바꾼다(둘 중 하나만).
- **확신도**: 높음 (양쪽 값과 엔진 분기 모두 소스로 대조)

### 2. 🟡 적 처치 드랍 경로가 동기 로드를 4번 연쇄한다 (데디 서버에서 비주얼 자산까지)
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:42`, 같은 파일 `:62`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:129`, `:134`
- **범주**: 성능/안전
- **문제**: `UWxRewardLibrary::GrantReward` 는 `Reward.Item.LoadSynchronous()`(42행)와 `PickupFragment->ItemActorClass.LoadSynchronous()`(62행)를, 그 뒤 `AWxItemPickup::SetItemDef`→`ApplyPickupVisual` 이 스태틱 메시(129행)와 Niagara 시스템(134행)을 다시 동기 로드한다. `GrantReward` 는 시작 시 1회가 아니라 **적이 죽을 때마다** 호출된다(`Source/WxGame/Character/WxEnemyCharacter.cpp:80`, `HandleDeath` 안). `WxInventoryManagerComponent.cpp:337` 의 "지급은 시작 시 1회 같은 산발적 호출이라 동기 로드해도 무방하다"는 근거가 이 경로에는 성립하지 않는다 — 전투 중 첫 드랍 프레임에서 히치가 난다.
  추가로 `ApplyPickupVisual` 은 서버 경로(`SetItemDef`, 56행)에서도 실행되므로 데디케이티드 서버가 메시·Niagara 같은 순수 클라 비주얼 자산을 로드한다.
- **제안**: `ItemActorClass`/`Mesh`/`NiagaraSystem` 은 전투 시작 전(스포너·Experience 로드 시점)에 `RequestAsyncLoad` 로 미리 워밍하거나, 최소한 `ApplyPickupVisual` 의 자산 로드를 `IsNetMode(NM_DedicatedServer)` 로 걸러 서버에서 건너뛴다.
- **확신도**: 높음 (호출 지점은 확인. 첫 로드 이후에는 캐시 히트라 히치는 아이템 종류당 1회로 한정된다)

### 3. 🟡 장비 경로 전체가 호출부 0건인 데드 코드인데 소비자만 배선돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h:236`(`EquipItemByDef`), 같은 헤더 `:180`(`RemoveItemInstance`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34`
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 는 저장소 전역 호출부 0건이고, 유일한 하위 호출인 `UWxEquipmentComponent::EquipItem` 도 그 외 진입이 없다. 둘 다 `BlueprintCallable` 이 아니라 BP 진입도 불가하다(`RemoveItemInstance` 역시 0건). 반면 소비자 쪽은 이미 붙어 있다 — `Source/WxGame/Character/WxCharacterBase.cpp:38` 이 모든 캐릭터에 `UWxEquipmentComponent` 를 생성하고 `:83` 이 `OnEquipVisualChanged` 를 구독한다. 결과적으로 복제 컴포넌트 하나가 전 캐릭터에 붙어 돌면서 아무 일도 하지 않고, `EquippedItemDef` 복제·`ActiveEquipEffectHandles` 수명·`BroadcastEquipVisual` 이 전부 미검증 상태로 남는다.
  경로를 살릴 때 함께 닫아야 할 구멍 둘: (a) `ApplyEquipEffects`(`:89`)가 ASC 해석 실패 시 조용히 반환하는데 `EquippedItemDef` 는 이미 확정된 뒤(`:51`)라 이후 ASC 가 준비돼도 재적용 기회가 없다. (b) 헤더 `WxEquipmentComponent.h:30` 이 이미 지적한 대로, 늦게 relevant 해진 클라는 초기 RepNotify 가 구독보다 앞서면 방송을 유실하는데 현재 상태를 되물을 pull API 가 없다.
- **제안**: 트리거를 실제로 배선하거나(인벤토리 UI/입력 → `EquipItemByDef`), 배선 계획이 없다면 `AWxCharacterBase` 의 컴포넌트 생성·구독까지 포함해 통째로 걷어낸다. 살리는 쪽이면 (a) ASC 미준비 시 `EquippedItemDef` 확정을 미루고, (b) 현재 메시/소켓을 되돌려주는 pull 함수를 추가해 구독 시점에 1회 당겨가게 한다.
- **확신도**: 높음 (호출부 0건은 저장소 전역 grep 으로 확인)

### 4. 🟡 StateTree 보상·리필 태스크가 0번 플레이어 컨트롤러로 고정돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 대상 선택에 `UGameplayStatics::GetPlayerController(Owner, 0)` 을 쓴다. 모듈 전체가 FastArray 델타 복제·`HasAuthority()` 게이팅으로 멀티플레이를 전제해 만들어졌는데 대상 선택만 싱글플레이 가정이다. 데디케이티드 서버에서 `GetPlayerController(World, 0)` 은 "로컬 플레이어"가 아니라 컨트롤러 이터레이터의 첫 항목이므로, 체크포인트 에스트병 리필이 임의의 한 명에게만 걸리고 비-픽업 보상(재화 등 `Pickup` Fragment 없는 항목)도 그 한 명에게만 직접 지급된다. `WxStateTreeTask_RefillItemCharges.h:21` 이 "로컬 플레이어(0번 컨트롤러)"라 적고 있어 의도된 단순화로 보이지만, 서버에는 로컬 플레이어라는 개념이 없다.
- **제안**: 리필은 `GetWorld()->GetPlayerControllerIterator()` 로 전 플레이어를 순회한다. 보상은 인스턴스 데이터에 바인딩 가능한 Actor 파라미터를 노출해 호출 측 ST 가 대상을 지목하게 한다. 현 형태를 유지한다면 헤더에 "싱글플레이 전용"임을 명시한다.
- **확신도**: 중간 (프로젝트가 의도적으로 싱글플레이 우선일 수 있다)

### 5. 🟢 `AddItemDefinition` 이 내부 엔트리 배열 참조를 든 채 루프 안에서 델리게이트를 방송한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:278`
- **범주**: 버그/정확성
- **문제**: 278행에서 `InventoryList.GetEntries()` 참조를 캐시한 뒤 279~302행 루프 안에서 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 로 임의의 구독자 코드를 실행한다. 구독자가 재진입으로 `AddItemDefinition`/`ConsumeItemsByDefinition` 을 호출하면 `Entries` 가 재할당·축소되어 캐시한 참조와 `EntryIndex` 가 모두 무효가 된다. 모듈 자체가 `FWxInventoryList::ConsumeByDefinition`(헤더 88행, "통지는 하지 않는다")에서 변경과 통지를 분리해 두었는데, 머지 루프만 그 규약을 벗어나 있다.
- **제안**: 머지 루프도 `FWxInventoryChangeResult` 배열로 변경 결과만 모으고 루프 종료 후 방송해 `ConsumeItemsByDefinition` 과 형태를 맞춘다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 현재 구독자는 표시용 뷰모델뿐이라 재진입 실적이 없다)

### 6. 🟢 `UWxItemFragment_Charges` 의 에디터 표시 이름이 `Refill` 이라 코드·문서와 어긋난다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:87`
- **범주**: 중복/복잡도
- **문제**: 클래스명·헤더 주석·README·매니저 API 가 모두 "Charges Fragment" 로 부르는데 디테일 패널에는 `Refill` 로 뜬다. 다른 5종은 모두 클래스명 접미사와 `DisplayName` 이 일치한다(`Equippable`/`Usable`/`Stackable`/`Pickup`/`Grade`). 기획자가 Fragment 를 고를 때만 이름이 달라 문서 대조가 끊긴다.
- **제안**: `DisplayName = "Charges"` 로 맞춘다.
- **확신도**: 높음

### 7. 🟢 픽업 프롬프트가 입력 키 `[F]` 를 문자열에 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:107`, `:108`
- **범주**: 설계/구조
- **문제**: `IWxInteractable::GetInteractionPrompt` 구현 4종 중 키를 문자열에 박은 것은 여기뿐이다(`AWxDevice` 는 ST 가 세팅한 프롬프트, `AWxDialogueActor` 는 컴포넌트 프롬프트, `AWxEnemyCharacter` 는 `"Finisher"`). 키를 리바인딩하면 픽업만 거짓 프롬프트를 띄우고, 도메인 플러그인이 입력 바인딩을 알아버리는 결합도 생긴다.
- **제안**: 프롬프트에서 `[F]` 를 빼고 키 표기는 표시 측(WxUI)이 EnhancedInput 바인딩에서 조립하게 한다.
- **확신도**: 중간 (전 프롬프트에 키를 붙이는 UI 정책이 아직 없어 임시로 넣어 둔 것일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`
- **경계 확인용으로 함께 읽은 모듈 밖 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`·`Private/WxInteractable.cpp`(픽업이 구현한 계약과 기본 구현), `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:50,86`(`OnInteracted` 가 서버 권위에서만 호출되어 `AddItemDefinition` 의 `check(HasAuthority())` 가 안전함을 확인), `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:69,77`(`UseItemByDef` 도 `HasAuthority` 가드 뒤에서만 호출됨), `Source/WxGame/MVVM/WxViewModel_Item.cpp:31,52`(뷰모델이 초기 충전량을 pull 하므로 클라 초기 charge 통지 누락이 증상으로 이어지지 않음을 확인), `Source/WxGame/Character/WxCharacterBase.cpp:38,83`, `Source/WxGame/Character/WxEnemyCharacter.cpp:80`, `Source/WxGame/Framework/WxGameMode.cpp:139`, `Config/DefaultGame.ini:43~50`
- **규칙 점검 결과(위반 0건)**: 22개 소스 전부 첫 줄 저작권 표기 정상, 람다 0건, `FORCEINLINE`/자유 인라인 정의 0건, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 한 곳뿐, 델리게이트 바인딩(`AddUObject`/`AddDynamic` 등) 자체가 0건이라 `Handle` 접두사 대상 없음(`OnRep_*` 는 RepNotify 라 해당 없음), `Build.cs`·`uplugin` 모두 `WxCore` 외 Wx 플러그인 참조 없음, override 의 `Super::` 호출 누락은 의도적 대체(`IsSupportedForNetworking`/`GetPrimaryAssetId`/순수 가상 `IWxInteractable`/StateTree `EnterState`) 외 없음. 헤더 인라인 정의는 템플릿 `FindFragmentByClass<T>`(Definition/Instance) 2건과 StateTree `GetInstanceDataType()` 2건뿐이며 넷 다 인접 주석에 규칙 6 예외 사유가 명시돼 있다.
- **미검토 / 한계**:
  - (a) **직전 리뷰의 🔴 는 반증되어 제거했다.** 그 발견은 "액터 채널이 컴포넌트 프로퍼티(=`InventoryList`)를 먼저 쓰고 서브오브젝트(`UWxItemInstance`)를 나중에 쓰므로 `PostReplicatedAdd` 시점에 `Entry.Instance` 가 unmapped 다"를 전제했는데, UE 5.8 `Engine/Private/DataChannel.cpp:4219~4230`(`UActorChannel::ReplicateRegisteredSubObjects`)은 정반대다 — 컴포넌트의 등록 서브오브젝트를 먼저 쓰고 컴포넌트 자신을 마지막에 쓰며, 엔진 주석이 그 이유를 `"SubObjects have to be created before the component on the receiving end"` 라고 못박고 있다. 레거시 경로(`AActor::ReplicateSubobjects`)도 동일 순서다. 따라서 `Entry.Instance` 는 FastArray 역직렬화 시점에 이미 해석돼 있고 통지 유실은 발생하지 않는다. 잔여 리스크는 `Entry.Instance->GetItemDef()`(하드 참조 데이터 자산)가 unmapped 인 경우뿐인데, `net.AllowAsyncLoading` 기본 0 에서는 패키지 맵이 동기 로드로 즉시 해석하므로 사실상 닫혀 있다.
  - (b) 위 (a) 와 발견 2·4 는 네트워크 PIE 실측을 하지 않았다 — 2인 데디케이티드 PIE 로 획득/드랍/리필 3경로를 한 번 훑어 두면 좋다.
  - (c) `AWxItemPickup` 의 물리 발사·이동 복제(`SetSimulatePhysics` + `SetReplicateMovement`)와, `FinishSpawning` 이전에 호출되는 `NiagaraComponent->Activate()` 의 미등록 컴포넌트 동작은 엔진 기본 경로에 맡기고 있어 별도 검증하지 않았다.
  - (d) `UWxItemFragment_Pickup::ItemActorClass` 가 가리키는 픽업 BP, 인벤토리 위젯, 보상 DataTable 내용 등 BP/에셋 내부 구조는 범위 밖이다.
  - (e) `UWxItemFragment_Grade` 의 에디터 전용 재시드(`PostEditChangeProperty`)는 코드만 읽고 에디터 실동작은 확인하지 않았다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 22파일 — `/module-review`로 갱신*
