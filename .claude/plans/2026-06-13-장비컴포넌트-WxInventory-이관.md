# UWxEquipmentComponent를 WxInventory로 이관

## 목표

`UWxEquipmentComponent`(현 `Source/WxGame/Component/`)가 한 클래스에 섞은 3개 책임 중, 인벤토리 도메인에 속한 상태(A)·EquipEffect GE 수명(B)을 WxInventory로 내리고, 무기 비주얼 스왑(C)만 캐릭터(WxGame)에 남긴다. 둘은 엔진 타입(`USkeletalMesh*`,`FName`)만 싣는 멀티캐스트 델리게이트로 연결해 WxInventory가 WxCombat/WxGame을 참조하지 않게 한다. 모듈 경계 감사 §4-1 해소.

| 책임 | 묶인 타입 | 위치 |
|---|---|---|
| A. 장착 상태(`EquippedItemDef` 복제) | `UWxItemDefinition`(WxInventory) | WxInventory |
| B. EquipEffect GE 수명 | `UWxItemFragment_Equippable.EquipEffects` + ASC | WxInventory(이미 GAS 의존) |
| C. 무기 비주얼 스왑 | `AWxWeaponBase`(WxCombat) + 캐릭터 `WeaponActor` ChildActorComponent(WxGame) | WxGame 캐릭터 |

## 변경 범위 (파일·모듈)

1. **WxInventory** — 컴포넌트 이관: `WxEquipmentComponent.{h,cpp}` → `Plugins/WxInventory/.../Public(Private)/Inventory/`. `WXGAME_API`→`WXINVENTORY_API`, `IWxEquipmentInterface` 상속 제거, `ApplyEquipmentVisuals` 삭제하고 프래그먼트에서 Mesh/Socket 추출 후 `OnEquipVisualChanged.Broadcast`. 델리게이트 `DECLARE_MULTICAST_DELEGATE_TwoParams(FWxOnEquipVisualChanged, USkeletalMesh*, FName)` 추가. WxGame/WxCombat include 제거.
2. **WxInventory.Build.cs** — `GameplayAbilities` Private→Public 승격(공개 헤더가 `FActiveGameplayEffectHandle` 노출).
3. **WxGame `AWxCharacterBase`** — `EquipmentComponent` 타입을 WxInventory 컴포넌트로, `IWxEquipmentInterface`/`EquipItem` 오버라이드 제거, `PostInitializeComponents`에서 델리게이트 바인딩, `HandleEquipVisualChanged(Mesh,Socket)` 신설(기존 비주얼 로직 동작 보존: `GetEquippedWeapon()->SetVisualMesh` + `WeaponActor` 소켓 재부착).
4. **WxInventory `WxInventoryManagerComponent.cpp`** — `EquipItemByDef`에서 인터페이스 Cast 대신 `Pawn->FindComponentByClass<UWxEquipmentComponent>()->EquipItem`.
5. **삭제** — `WxEquipmentInterface.h`.
6. **문서(비차단)** — `WxInventory/README.md`, 감사 문서 §4-1 갱신.

## 접근 방식

컴포넌트가 자기 프래그먼트에서 엔진 타입만 뽑아 델리게이트로 방송(서버 EquipItem·클라 OnRep) → 캐릭터가 바인딩해 자기 `WeaponActor`와 무기 공개 API로 반영. 경계를 넘는 데이터는 `USkeletalMesh*`/`FName`뿐. WxInventory.Build.cs는 여전히 Wx 중 WxCore만, WxCombat 무변경.

## 기각한 대안

- WxCombat 배치 → `WxCombat→WxInventory` 위반.
- 비주얼까지 WxInventory → 무기/캐릭터 ChildActorComponent 못 봄, 불가.
- WxCore `IWxEquipmentVisual` 신설 → 델리게이트로 충분, 과한 추상화.
- 무기 액터 `ApplyEquipVisual` → 소켓 재부착은 캐릭터 ChildActorComponent 재부모화라 WxGame 책임.

## 검증

WxEditor(Development) 빌드 → PIE 장착(메시/소켓/GE 적용·해제) → 리슨서버 멀티 OnRep 비주얼 → WxInventory 소스에 WxGame/WxCombat include 없음(grep).
