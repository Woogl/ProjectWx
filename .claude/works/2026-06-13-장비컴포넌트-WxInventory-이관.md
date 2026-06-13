# UWxEquipmentComponent를 WxInventory로 이관 (완료 기록)

대응 plan: `.claude/plans/2026-06-13-장비컴포넌트-WxInventory-이관.md`

## 작업 개요

`UWxEquipmentComponent`가 WxGame(컴포지션 루트)에서 한 클래스에 섞고 있던 3개 책임 중, 인벤토리 도메인에 속한 **장착 상태(`EquippedItemDef` 복제) + EquipEffect GE 수명**을 WxInventory로 내리고, **무기 비주얼 스왑**만 캐릭터(WxGame)에 남겼다. 둘은 엔진 타입(`USkeletalMesh*`,`FName`)만 싣는 멀티캐스트 델리게이트 `OnEquipVisualChanged`로 연결해 WxInventory가 WxCombat/WxGame을 일절 참조하지 않게 했다. 모듈 경계 감사 §4-1 해소. WxEditor(Development) 빌드 통과.

## 변경한 파일·모듈

**WxInventory**
- `Public/Inventory/WxEquipmentComponent.h`, `Private/Inventory/WxEquipmentComponent.cpp` — 신규(구 WxGame 파일에서 이관). `WXINVENTORY_API`, `IWxEquipmentInterface` 상속 제거, `ApplyEquipmentVisuals` 삭제 → `BroadcastEquipVisual`(프래그먼트에서 Mesh/Socket 추출 후 델리게이트 방송). `OnEquipVisualChanged` 추가.
- `Private/Inventory/WxInventoryManagerComponent.cpp` — `EquipItemByDef`가 인터페이스 Cast 대신 `Pawn->FindComponentByClass<UWxEquipmentComponent>()->EquipItem`. include 교체 + `GameFramework/Pawn.h` 추가.
- `Public/Inventory/WxInventoryManagerComponent.h` — `EquipItemByDef` 주석 갱신.
- `WxInventory.Build.cs` — `GameplayAbilities` Private→Public 승격.
- `Public/Inventory/WxEquipmentInterface.h` — **삭제**.

**WxGame**
- `Character/WxCharacterBase.h/.cpp` — `EquipmentComponent` 타입을 WxInventory 컴포넌트로, `IWxEquipmentInterface`/`EquipItem` 오버라이드 제거, `PostInitializeComponents`에서 `OnEquipVisualChanged` 바인딩, `HandleEquipVisualChanged(MeshAsset, Socket)` 신설.
- `Component/WxEquipmentComponent.h/.cpp` — **삭제**.

**Config / 문서**
- `Config/DefaultEngine.ini` — `[CoreRedirects]`에 클래스 이동 리다이렉트 추가.
- `Plugins/WxInventory/README.md`, `Docs/Programmer/Module_Boundary_Audit.md`(§4-1 해소), `Docs/Programmer/Inventory_Lyra_Comparison.md` 갱신.

## 구현·결정 사항과 그 이유

- **WxInventory 배치(WxCombat 아님)**: 컴포넌트 입력·진실원본이 `UWxItemDefinition`/`UWxItemFragment_Equippable`(WxInventory). WxCombat에 두면 `WxCombat→WxInventory` 위반. 전투는 GE/어트리뷰트로만 결합돼 있어 장비 존재를 몰라도 됨.
- **비주얼은 캐릭터(WxGame)**: 소켓 재부착이 캐릭터의 `WeaponActor`(ChildActorComponent, WxGame 소유)를 재부모화하는 동작이라 무기 액터가 아니라 캐릭터의 책임. 무기 메시 교체는 무기 공개 API `SetVisualMesh` 재사용.
- **델리게이트 시그니처(엔진 타입)**: `(USkeletalMesh*, FName)`만 실어 프래그먼트 타입 노출 없이 경계를 넘김. C++ 바인딩 전용이라 비-dynamic `DECLARE_MULTICAST_DELEGATE_TwoParams`.
- **인터페이스 제거**: 컴포넌트가 WxInventory로 오면서 매니저가 구체 컴포넌트를 직접 조회 가능 → `IWxEquipmentInterface`가 잉여가 되어 삭제(불필요한 간접 지양).
- **GameplayAbilities Public 승격**: 공개 헤더가 `FActiveGameplayEffectHandle`(private 멤버 타입)을 노출하므로 계약상 Public이 맞음.
- **방송 타이밍 보존**: 서버 `EquipItem`·클라 `OnRep_EquippedItemDef` 양쪽에서 방송 → 기존 동작과 동일, 회귀 없음. 무기 미스폰 시 early-return(다음 사이클 재시도)도 유지.

## plan 대비 달라진 점

- **CoreRedirect 추가(plan 외)**: UCLASS 모듈 이동의 표준 안전장치로 `Config/DefaultEngine.ini`에 `+ClassRedirects` 추가. 스냅샷/콘텐츠에 직접 참조는 없었으나 직렬화된 참조 대비 보험.
- **C4458 수정**: 핸들러 파라미터명 `Mesh`가 `ACharacter::Mesh` 멤버를 가려 `-WarningsAsErrors`로 빌드 1차 실패 → `MeshAsset`으로 변경 후 통과.

## 후속 과제

- 런타임 검증(PIE/리슨서버 최대 4인): 장착 시 무기 메시 스왑·소켓 정확, EquipEffects GE 적용/해제, 원격 클라 `OnRep` 외형 동기화 확인. (현재 컴파일까지만 검증)
- 경계 감사 §4-2(어빌리티 아이콘 리졸버), §6(`FWxCharacterUIData`/`EWxTeam` WxCore 이동)은 별도 작업으로 남음.
