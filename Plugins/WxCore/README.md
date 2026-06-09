# WxCore — 공용 정의 모듈

> 프로젝트 전체 플러그인이 공유하는 공용 정의(Gameplay Tag, 콜리전 채널, 도메인 간 인터페이스)를 한곳에 모아두는 foundation 모듈. 구현은 두지 않고 선언/상수/계약만 제공한다.

## 책임
**담당**
- 프로젝트 전체에서 쓰는 Gameplay Tag의 C++ Native Tag 선언 (`WxGameplayTags`)
- 커스텀 콜리전 채널 상수 정의 (`WxCollision::WxAttack`)
- 도메인 간 결합을 끊기 위한 공용 인터페이스 선언 (`IWxSavable`)

**경계 (비담당)**
- 실제 시스템 구현 일체. foundation 규칙상 직접 구현을 두지 않는다. 전투는 [[WxCombat]], 세이브 로직은 [[WxSave]], 월드 오브젝트는 [[WxWorld]] 등이 담당
- Gameplay Tag를 dispatch/소비하는 어빌리티·이펙트 로직은 각 도메인 모듈에 위치
- 콜리전 채널의 실제 ini 등록은 프로젝트 설정(DefaultEngine.ini)이 담당

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 선언용). 그 외는 빌드 기본(Core/CoreUObject)뿐
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 프로젝트 전 영역 Gameplay Tag 선언부 (State/Event/ANS/Cue/Damage/Ability/Input/SetByCaller/UI) | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `WxCollision` (namespace) | 커스텀 콜리전 채널 상수. `WxAttack = ECC_GameTraceChannel1` | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxSavable` | WxSave 슬롯 저장/로드 라이프사이클 참여 마커 + 후크 (`GetWxSaveId`, `OnWxSaveRestored`) | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `FWxCoreModule` | 모듈 진입점 (Startup/Shutdown) | `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` (정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`)
- 주요 네임스페이스:
  - `State.*` — 캐릭터 상태 (Dead, Groggy, LockOn, Recognized, Invincible, Guard, PerfectGuard, HitReact, SuperArmor), 주로 ASC에 부여
  - `Event.*` — GameplayEvent dispatch 태그 (HitReact 계열, DodgeSuccess, PerfectGuard)
  - `ANS.*` — AnimNotifyState 구간 (WeaponCollision, ComboWindow)
  - `GameplayCue.*` — Cue 트리거 (Damage, BuffATK, Exceed, Burn, HitStop, Metamorphose 등)
  - `Damage.*` — 대미지 판정 결과/속성 (Critical, Unblockable, ParryHitReact)
  - `Ability.*` / `Input.*` — 어빌리티·입력 매핑 (Attack, Dodge, Sprint, Guard, Skill_N, Ultimate, Pattern_N 등)
  - `SetByCaller.*` — GE SetByCaller 키 (Duration, Cost, Recovery_UP/MP, ReflectDP, Coeff_ATK, RawDamage)
  - `UI.Layer.*` — UI 레이어 스택 (Game/GameMenu/Menu/Modal)

## 확장 포인트 / 규약
- 태그 추가: `WxGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `WxGameplayTags.cpp`에 정의를 같이 작성. 변수명은 점(.)을 언더스코어(_)로 치환 (`State.Dead` → `State_Dead`). 다른 모듈에서 임의 선언 금지
- 세이브 대상 액터: `IWxSavable`을 구현하고 `GetWxSaveId()`로 세션 불변 `FGuid`를 반환(보존 필드에 `UPROPERTY(SaveGame)` 표시). 복원 후처리는 `OnWxSaveRestored()` 오버라이드. 인터페이스를 WxCore에 둠으로써 WxSave ↔ 소비 도메인(예: WxWorld)의 직접 의존을 끊는다
- 콜리전 채널 추가 시 `DefaultEngine.ini`의 채널 등록 순서와 `WxCollisionChannels.h` 상수가 일치해야 함

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전체 태그 체계를 한눈에 파악 (각 태그 주석에 소비처 명시되어 시스템 색인 역할)
2. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 도메인 결합 분리 패턴의 대표 예시

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxQuest]], [[WxSave]])과 게임 모듈 [[WxGame]]이 WxCore를 참조

---
*문서 기준 커밋 `80cc348` · 생성일 2026-06-09 · 소스 6파일 — `/readme-writer`로 갱신*
