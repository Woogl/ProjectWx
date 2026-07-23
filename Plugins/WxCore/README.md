# WxCore — 공용 정의 모듈

> 모든 Wx 도메인 플러그인이 공유하는 공용 정의(Gameplay Tag, 콜리전 채널, 도메인 간 인터페이스)를 한곳에 모은 foundation 모듈. 구현 로직은 두지 않고 선언·상수·계약만 제공한다.

## 책임
**담당**
- 프로젝트 전역 Gameplay Tag의 C++ Native Tag 선언·정의 (`WxGameplayTags`)
- 커스텀 콜리전 채널 상수 (`ECC_WxAttack`, `ECC_WxInteractable`)
- 도메인 간 결합을 끊는 공용 인터페이스 (`IWxSavable`, `IWxInteractable`)

**경계 (비담당)**
- 시스템 구현 일체 — 전투 [[WxCombat]], 세이브 로직 [[WxSave]], 상호작용/월드 오브젝트 [[WxWorld]], UI [[WxUI]]
- 태그를 dispatch·소비하는 어빌리티·이펙트 로직은 각 도메인 모듈에 위치
- `IWxSavable`/`IWxInteractable` 구현체는 각 도메인이 상속해 정의
- 콜리전 채널의 실제 ini 등록·프로파일 응답은 프로젝트 설정(`Config/DefaultEngine.ini`)

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 매크로). 그 외는 `Core`/`CoreUObject`/`Engine` 기본만 — GAS(GameplayAbilities) 등 어떤 도메인 서브시스템에도 의존하지 않는다.
- 규칙: WxCore는 다른 Wx 플러그인 참조 금지 — 없음 ✅ (`Plugins/WxCore/Source/WxCore/WxCore.Build.cs`의 의존은 엔진 모듈뿐, `Plugins/WxCore/WxCore.uplugin`에 Plugins 의존 0)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 프로젝트 전역 Native Tag 선언부 (State/Event/Gimmick/ANS/Cue/Damage/Ability/SetByCaller/UI). 다른 모듈이 참조하는 어휘집 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `ECC_WxAttack` / `ECC_WxInteractable` | 커스텀 콜리전 채널 상수 (`ECC_GameTraceChannel1`/`ECC_GameTraceChannel2`). ini 등록과 동기화되는 단일 출처 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxSavable` | WxSave 슬롯 저장/로드 라이프사이클 참여 마커+후크 (`GetSaveId`, `OnWxSaveRestored`). WxSave↔소비 도메인 직접 의존 차단 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `IWxInteractable` | 상호작용 대상의 공용 계약 (`OnInteracted` 응답, `GetInteractionPrompt`). 대상 액터가 직접 구현 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `FWxCoreModule` | 모듈 진입점 (Startup/Shutdown 비어 있음, 별도 부트스트랩 없음) | `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
이 모듈이 프로젝트의 유일한 C++ Native Tag 선언처다.
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 태그 추가 시 위 두 파일에만 작성. 변수명은 점(`.`)을 언더스코어(`_`)로 치환 (`State.Dead` → `State_Dead`).
- 주요 네임스페이스:
  - `State.*` — 캐릭터/ASC 상태 (Dead, Ragdoll, Groggy, LockOn, LockedOn, InCombat, Invincible, Guard, PerfectGuard, HitReact, SuperArmor, Finisher)
  - `Event.*` — GameplayEvent dispatch 태그 (HitReact 계열, DodgeSuccess, PerfectGuard, UseItem, Interact, Finisher/Backstab, HitStop 역경직)
  - `Gimmick.*` — 월드 기믹의 권위 상태값이자 GimmickStateTree 진입 이벤트 겸용 (Door/Elevator/SpawnConsole/AlarmConsole/CutsceneTrigger/TreasureChest/CheckPoint/LaserCorridor) + `Gimmick.Restore` 세이브 복원 마커
  - `ANS.*` — AnimNotifyState 구간 (WeaponCollision, ComboWindow)
  - `GameplayCue.*` — Cue 트리거 (Damage, PerfectGuard, Exceed, Burn, AttackTelegraph 색상별 Red/Yellow/Blue/Purple)
  - `Damage.*` — 대미지 판정 결과/속성 (Critical, Unblockable, ParryHitReact)
  - `Ability.*` — 어빌리티 식별 (Attack, Dodge, Sprint, Guard, Skill_N, Ultimate, Interact, UseItem, AI Pattern_N)
  - `SetByCaller.*` — GE SetByCaller 키 (Duration, Recovery_UP/MP, ReflectDP, Coeff_ATK, RawDamage, HitStop)
  - `UI.Layer.*` / `UI.Action.*` — CommonUI 레이어 스택(Game/GameMenu/Menu/Modal) 및 액션(Inventory/MainMenu/FreeCursor)

## 확장 포인트 / 규약
- 태그 추가: `WxGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `WxGameplayTags.cpp`에 `UE_DEFINE_GAMEPLAY_TAG`를 쌍으로 작성. 다른 모듈에서 임의 선언 금지.
- 세이브 대상 액터: `IWxSavable` 구현 + 보존 필드에 `UPROPERTY(SaveGame)` 표시. `GetSaveId()`가 세션 불변 `FGuid`를 반환(무효 GUID면 저장/복원 제외), 복원 후처리는 `OnWxSaveRestored()` 오버라이드(BeginPlay 이전 호출 가능). 인터페이스를 WxCore에 둠으로써 WxSave↔소비 도메인 직접 의존을 끊는다. (구현은 BP 불가 — `CannotImplementInterfaceInBlueprint`)
- 상호작용 대상: `IWxInteractable`로 소비 도메인이 WxWorld 구현체에 의존하지 않고 자기 액터를 상호작용 대상으로 구현. `OnInteracted` 응답은 서버 권위 호출, `GetInteractionPrompt` 는 레지스트리가 pull.
- 콜리전 채널 추가: `ECC_Wx*` 상수와 `Config/DefaultEngine.ini`의 채널 등록 순서가 일치해야 함.
- WxCore엔 정의/공용 계약만 둔다. 리플리케이션·권한 로직은 소비 도메인이 책임진다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 이 모듈의 핵심. 프로젝트 전역 태그 어휘와 각 태그 주석의 소비처가 시스템 색인 역할을 한다.
2. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` / `WxInteractable.h` — 도메인 간 직접 의존을 끊는 앵커 인터페이스 패턴의 대표 예시.

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxSound]], [[WxAI]], [[WxQuest]], [[WxSave]])과 게임 모듈 [[WxGame]]이 WxCore를 참조

---
*문서 기준 커밋 `10f1722` · 생성일 2026-07-23 · 소스 7파일 — `/readme-writer`로 갱신*
