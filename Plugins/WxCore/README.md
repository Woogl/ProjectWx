// Copyright Woogle. All Rights Reserved.

# WxCore — 공용 정의 모듈

> 프로젝트 전체 플러그인이 공유하는 공용 정의(Gameplay Tag, 콜리전 채널, 도메인 간 인터페이스)를 한곳에 모아두는 foundation 모듈. 구현은 두지 않고 선언/상수/계약만 제공한다.

## 책임
**담당**
- 프로젝트 전체에서 쓰는 Gameplay Tag의 C++ Native Tag 선언 (`WxGameplayTags`)
- 커스텀 콜리전 채널 상수 정의 (`WxCollision::WxAttack`)
- 도메인 간 결합을 끊기 위한 공용 인터페이스 선언 (`IWxSavable`)

**경계 (비담당)**
- 실제 시스템 구현 일체. foundation 규칙상 직접/간접 구현 금지. 전투는 [[WxCombat]], 세이브 로직은 [[WxSave]], 월드 오브젝트는 [[WxWorld]] 등이 담당
- Gameplay Tag를 dispatch/소비하는 어빌리티·이펙트 로직은 각 도메인 모듈에 위치

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 선언용). Wx 모듈 의존 없음
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 프로젝트 전 영역 Gameplay Tag 선언부 (State/Event/ANS/Cue/Damage/Ability/Input/Item/UI 등) | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `WxCollision` (namespace) | 커스텀 콜리전 채널 상수. `WxAttack = ECC_GameTraceChannel1` | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxSavable` | WxSave 슬롯 저장/로드 라이프사이클 참여 마커 + 후크 (`GetWxSaveId`, `OnWxSaveRestored`) | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `FWxCoreModule` | 모듈 진입점 (Startup/Shutdown 비어 있음) | `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` (정의: `Private/WxGameplayTags.cpp`)
- 주요 네임스페이스:
  - `State.*` — 캐릭터 상태 (Dead, Aerial, Groggy, LockOn, Invincible, Guard, SuperArmor 등)
  - `Event.*` — 이벤트 dispatch 태그 (HitReact 계열, DodgeSuccess, PerfectGuard)
  - `ANS.*` — AnimNotifyState 구간 (WeaponCollision, ComboWindow)
  - `GameplayCue.*` — Cue 트리거 (Damage, Burn, HitStop, Metamorphose 등)
  - `Damage.*` — 대미지 판정 결과/속성 (Critical, Unblockable, ParryHitReact)
  - `Ability.*` / `Input.*` — 어빌리티·입력 매핑 (Attack, Dodge, Skill_N, Pattern_N 등)
  - `SetByCaller.*` — GE SetByCaller 키 (Duration, Cost, RawDamage 등)
  - `Item.*` / `UI.*` — 재화, UI 레이어 (Layer_Game/GameMenu/Menu/Modal)

## 확장 포인트 / 규약
- 태그 추가: `WxGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `WxGameplayTags.cpp`에 정의를 같이 작성. 변수명은 점(.)을 언더스코어(_)로 치환 (`State.Dead` → `State_Dead`)
- 콜리전 채널 추가 시 `DefaultEngine.ini`의 채널 등록 순서와 상수가 일치해야 함
- 도메인 간 직접 의존을 끊어야 할 계약은 인터페이스로 이곳에 두어, 양쪽 도메인이 WxCore만 참조하게 한다 (예: WxSave ↔ WxWorld 분리를 위한 `IWxSavable`)

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전체 태그 체계를 한눈에 파악 (각 태그 주석에 소비처 명시)
2. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 도메인 결합 분리 패턴의 대표 예시

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxQuest]], [[WxSave]])과 게임 모듈 [[WxGame]]이 WxCore를 참조

---
*문서 기준 커밋 `59bfe3f` · 생성일 2026-06-08 · 소스 6파일 — `/readme-writer`로 갱신*
