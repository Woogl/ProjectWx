# WxCore — 공용 정의 파운데이션

> 모든 Wx 플러그인이 공유하는 최하위 정의 계층. 공용 Gameplay Tag, 커스텀 콜리전 채널, 그리고 도메인 간 직접 결합을 끊는 계약 인터페이스(`IWxInteractable`·`IWxSavable`)를 한곳에 둔다. 구현 로직은 두지 않고 선언·상수·계약만 제공한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag의 단일 선언처 (`WxGameplayTags`)
- 커스텀 콜리전 채널 상수 정의 (`ECC_WxAttack`)
- 도메인 간 직접 의존을 끊는 공용 계약 인터페이스: 상호작용(`IWxInteractable`), 세이브 참여(`IWxSavable`)
- 여러 도메인 StateTree 노드가 공유하는 값 래퍼 (`FWxActorTarget`)

**경계 (비담당)**
- 상호작용 스캐너·어빌리티 실제 구현 — 계약만 제공, 소비는 [[WxWorld]]·[[WxCombat]]
- 세이브 슬롯 직렬화·복원 오케스트레이션 — [[WxSave]]
- 위 태그를 실제로 부여/dispatch/수신하는 어빌리티·이펙트 로직 — 각 도메인
- 콜리전 채널의 ini 등록·프로파일 응답 — 프로젝트 설정(`Config/DefaultEngine.ini`)

## 의존성
- **주요 의존**: 없음(Wx 플러그인 무참조). 엔진 `GameplayTags`(Native Tag 매크로), `UniversalObjectLocator`(FWxActorTarget)에만 의존. GAS 등 어떤 도메인 서브시스템에도 의존하지 않는다.
- 규칙: foundation 모듈로서 다른 Wx 플러그인을 참조하지 않아야 함 — `WxCore.Build.cs`의 `PublicDependencyModuleNames`는 `Core`/`CoreUObject`/`Engine`/`GameplayTags`/`UniversalObjectLocator`뿐, `WxCore.uplugin`에 Plugins 의존 0 → **준수**.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 프로젝트 전역 Native Tag 선언부. 다른 모듈이 참조하는 상태/이벤트 어휘집 | `Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 대상 액터가 직접 구현, 소비처는 `Find`로 조회 | `Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 라이프사이클 참여 마커 + 후크(`GetSaveId`/`OnSaveRestored`) | `Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 커스텀 Object Channel 상수(`ECC_GameTraceChannel1`) | `Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxActorTarget` | UOL을 감싸 ST 인스턴스 데이터 픽커 제한(5.8)을 우회하는 래퍼 | `Source/WxCore/Public/WxActorTarget.h` |
| `FWxCoreModule` | 모듈 진입점(Startup/Shutdown, 별도 부트스트랩 없음) | `Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
이 모듈이 프로젝트의 유일한 C++ Native Tag 선언처다.
- 선언: `Source/WxCore/Public/WxGameplayTags.h` / 정의: `Source/WxCore/Private/WxGameplayTags.cpp` (태그 추가 시 이 두 파일에만 작성)
- 네이밍 규약: 태그 문자열의 점(`.`)을 언더스코어(`_`)로 치환한 변수명. 예) `"State.Dead"` → `State_Dead`
- 주요 네임스페이스:
  - `State.*` — 캐릭터/ASC 상태(Dead, Ragdoll, Groggy, LockOn, LockedOn, InCombat, Invincible, Guard, PerfectGuard, HitReact, SuperArmor, Finisher, Dialogue)
  - `Event.*` — GameplayEvent dispatch 태그(HitReact 계열, DodgeSuccess, PerfectGuard, UseItem, Interact, Finisher/Backstab, HitStop 역경직)
  - `Gimmick.*` — 월드 기믹의 권위 상태값이자 GimmickStateTree 진입 이벤트 겸용(Door/Elevator/SpawnConsole/AlarmConsole/CutsceneTrigger/TreasureChest/CheckPoint/LaserCorridor). 부모 태그 `Gimmick`이 ST Root 재선택 전이를 받으므로 이 계층에는 **상태 태그만** 둔다
  - `StateTree.Restore` — 세이브 복원 시 일회성 효과를 스냅 처리하는 공용 복원 마커(상태가 아니므로 `Gimmick.*` 밖에 산다)
  - `ANS.*`(ComboWindow), `GameplayCue.*`(Damage/PerfectGuard/Exceed/Burn/AttackTelegraph 색상별), `Damage.*`(Critical/Unblockable/ParryHitReact)
  - `Ability.*`(Attack/Dodge/Sprint/Guard/Skill_N/Ultimate/Interact/UseItem, AI Pattern_N), `SetByCaller.*`(Duration/Recovery_UP·MP/Coeff_ATK/RawDamage/HitStop)
  - `UI.Layer.*`(Game/GameMenu/Menu/Modal) / `UI.Action.*`(Inventory/MainMenu/FreeCursor) — CommonUI 레이어 스택·액션

## 확장 포인트 / 규약
- **태그 추가**: `WxGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `WxGameplayTags.cpp`에 정의를 쌍으로 작성. 다른 모듈에서 임의 선언 금지.
- **상호작용 대상 만들기**: 대상 액터가 `IWxInteractable`를 구현(BP 불가 — `CannotImplementInterfaceInBlueprint`)하고 `IsInteractionMeshActive`(어느 메시가 지금 켜진 영역인가 — 표식과 활성을 겸함)·`OnInteracted`(서버 권위 응답)·`GetInteractionPrompt`(스캐너 pull)를 채운다. 셋 다 `Source` 메시를 받아 한 액터의 여러 영역(예: 엘리베이터)을 가른다. 주체별 자격이 갈리면(예: 뒤잡) `CanBeInteractedBy` 오버라이드. 소비처는 항상 `IWxInteractable::Find(Mesh)`를 거치므로 계약이 액터→컴포넌트로 내려가도 조회 지점 한 곳만 바뀐다. 영역 메시엔 쿼리 콜리전 필요(감지·사거리를 형상으로 잰다).
- **세이브 참여**: 액터가 `IWxSavable` 구현 + 보존 필드에 `UPROPERTY(SaveGame)` 표시. `GetSaveId()`는 세션 불변 `FGuid` 반환(무효 GUID면 저장/복원 제외 — `GetActorGuid`가 에디터 전용이라 영속 UPROPERTY로 대체), 복원 후처리는 `OnSaveRestored()`(BeginPlay 이전 호출 가능).
- **계약이 WxCore에 있는 이유**: 소비 도메인(예: WxInventory 픽업, WxWorld 기믹)이 WxWorld·WxSave에 직접 의존하지 않고도 계약을 구현하게 하는 결합 차단 장치.
- **콜리전 채널 추가**: `ECC_Wx*` 상수와 `Config/DefaultEngine.ini` 채널 등록 순서가 일치해야 함.

## 여기서부터 읽어라
1. `Source/WxCore/Public/WxGameplayTags.h` — 이 모듈의 핵심. 각 태그 주석의 소비처가 사실상 전 프로젝트 시스템 색인 역할을 한다.
2. `Source/WxCore/Public/WxInteractable.h` — 계약 인터페이스가 도메인 결합을 어떻게 끊는지 보여주는 대표 예. `WxInteractable.cpp`의 `Find`/`IsMeshInRange`도 함께.
3. `Source/WxCore/Public/WxSavable.h` — 세이브 시스템 참여 규약.

## 관련
- 상위(소비): 모든 Wx 도메인 플러그인([[WxCombat]] 태그·Event·SetByCaller, [[WxWorld]] `IWxInteractable`·`Gimmick.*`, [[WxSave]] `IWxSavable`·`StateTree.Restore`, [[WxUI]] `UI.Layer/Action.*`, [[WxQuest]] `Quest.Fail`, [[WxInventory]]·[[WxAI]]·[[WxDialogue]])과 게임 모듈 [[WxGame]]이 WxCore를 참조.

---
*문서 기준 커밋 `a5b5f20` · 생성일 2026-07-29 · 소스 9파일 — `/readme-writer`로 갱신*
